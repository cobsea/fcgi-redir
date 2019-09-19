#include "cgi_handler_thread.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcgi_config.h>
#include <fcgiapp.h>
#include <mysql.h>
#include <mysqld_error.h>
#include <libmemcached/memcached.h>

#include "cgi_query_parser.h"
#include "meta_structs.h"

const char *incorrect_query_error_answer = "Status: 502 Incorrect Query\n"
    "\n<!DOCTYPE html><html><head><title>502 Incorrect query</title></head><body>"
    "<center><h1>Error 502: Incorrect query.</h1></center><br>\n"
    "<h3>Proper URL looks like this:</h3><br>\n"
    "<h3>http://1.2.3.4/app/fcgi_redir?username=Anil&ran=0.97584378943&pageURL=http://xyz.com</h3>"
    "</body></html>";

const char *database_connection_error_answer = "Status: 502 Database Connection Error\n"
    "\n<!DOCTYPE html><html><head><title>502 Database Connection Error</title></head><body>"
    "<center><h1>Error 502: Database Connection Error.</h1></center><br>\n"
    "<h3>:(</h3><br>\n"
    "</body></html>";

void *cgi_handler_thread(void *args) {
  // deducing shared arguments
  int socket_fd = *((int*)(((void**)args)[0]));
  struct mysql_metadata mysql = *((struct mysql_metadata*)(((void**)args)[1]));
  struct memcached_metadata memcached_conf = *((struct memcached_metadata*)(((void**)args)[2]));
  FILE *log_file = (FILE*)(((void**)args)[3]);
  pthread_mutex_t *request_mutex_ptr = (pthread_mutex_t*)(((void**)args)[4]);
  pthread_mutex_t *log_mutex_ptr = (pthread_mutex_t*)(((void**)args)[5]);
  bool *finish_threads_ptr = (bool*)(((void**)args)[6]);

  // connecting to MySQL service
  MYSQL *db_connection = mysql_init(NULL);
  if (db_connection == NULL) {
    fprintf_threaded(log_mutex_ptr, log_file, "Coudn't init mysql in thread\n");
  }
  else {
    bool reconnect = true;
    mysql_options(db_connection, MYSQL_OPT_RECONNECT, &reconnect); // turning the reconnection on
    if (mysql_real_connect(db_connection, mysql.host, mysql.user,
        mysql.password, mysql.database, mysql.port, NULL, 0) == NULL)
    {
      fprintf_threaded(log_mutex_ptr, log_file, "Coudn't connect to database\n");
    }
    else {
      // connecting to memcached daemon
      char *memcached_config_str = (char*)malloc(
        (strlen(memcached_conf.host) + strlen(memcached_conf.port) + 12) * sizeof(char)
      );
      sprintf(memcached_config_str, "--SERVER=%s:%s", memcached_conf.host, memcached_conf.port);
      memcached_st *memc = memcached(memcached_config_str, strlen(memcached_config_str));
      if (memc == NULL) {
        fprintf_threaded(log_mutex_ptr, log_file, "Coudn't connect to memcached\n");
        // not so fatal error, so continue working with db-only
      }

      // initializing FastCGI
      FCGX_Request request;
      if (FCGX_InitRequest(&request, socket_fd, 0) != 0) {
        fprintf_threaded(log_mutex_ptr, log_file, "Coudn't initiate request\n");
      }
      else {
        while (!(*finish_threads_ptr)) {
          pthread_mutex_lock(request_mutex_ptr);
          if (FCGX_Accept_r(&request) < 0) {
            fprintf_threaded(log_mutex_ptr, log_file, "Coudn't accept request\n");
            pthread_mutex_unlock(request_mutex_ptr);
            break;
          }
          pthread_mutex_unlock(request_mutex_ptr);

          // copying and analyzing cgi query
          char *cgi_query_string = FCGX_GetParam("QUERY_STRING", request.envp);
          struct cgi_query *cgi_query = deduce_cgi_args(cgi_query_string);

          char *final_url = NULL; // the string which user will be redirected to
          bool fetched_from_memcached = false;
          int redirection_status = 0; // 0 = no redirection, 1 = redirection to target url, 2 = redirection to fallback url
          memcached_return_t memc_return_status;
          enum net_async_status db_async_status;
          char *db_query_string = NULL;

          FCGX_PutS("Content-Type: text/html\n"
                    "Connection: close\n", request.out);

          // checking if connection is online
          if (mysql_ping(db_connection) != 0) {
            FCGX_PutS(database_connection_error_answer, request.out);
            fprintf_threaded(log_mutex_ptr, log_file, "Coudn't reconnect to MySQL\n");
            redirection_status = 0;
          }
          else {
            if (cgi_query == NULL) {
              FCGX_PutS(incorrect_query_error_answer, request.out);
              redirection_status = 0;
            }
            else {
              db_query_string = (char*)malloc((37 + strlen(cgi_query->username)) * sizeof(char));
              sprintf(db_query_string, "SELECT url FROM pairs WHERE name='%s'", cgi_query->username);

              db_async_status = mysql_real_query_nonblocking(
                  db_connection, db_query_string, (unsigned long)strlen(db_query_string)
              );
              size_t memc_value_size;
              uint32_t memc_value_flags;
              char *memc_fetched_url = memcached_get(
                  memc, cgi_query->username, strlen(cgi_query->username), &memc_value_size, &memc_value_flags, &memc_return_status
              );

              if (memc_return_status == MEMCACHED_SUCCESS) {
                // redirection to url from memcached
                final_url = (char*)malloc((strlen(memc_fetched_url) + 1) * sizeof(char));
                strcpy(final_url, memc_fetched_url);
                fetched_from_memcached = true;
                redirection_status = 1;
              }
              else {
                while (db_async_status == NET_ASYNC_NOT_READY) {
                  db_async_status = mysql_real_query_nonblocking(
                      db_connection, db_query_string, (unsigned long)strlen(db_query_string)
                  );
                }
                if (db_async_status == NET_ASYNC_ERROR) {
                  fprintf_threaded(log_mutex_ptr, log_file, "Coudn't send mysql query\n");
                  FCGX_PutS(database_connection_error_answer, request.out);
                  redirection_status = 0;
                }
                else {
                  MYSQL_RES *db_result = mysql_store_result(db_connection);
                  if (db_result == NULL) {
                    fprintf_threaded(log_mutex_ptr, log_file, "Coudn't receive mysql result\n");
                    FCGX_PutS(database_connection_error_answer, request.out);
                    redirection_status = 0;
                  }
                  else {
                    MYSQL_ROW row = mysql_fetch_row(db_result);
                    if (row == NULL) {
                      // redirection to fallback url
                      final_url = (char*)malloc((strlen(cgi_query->pageURL) + 1) * sizeof(char));
                      strcpy(final_url, cgi_query->pageURL);
                      redirection_status = 2;
                    }
                    else {
                      if (!is_url(row[0])) {
                        // redirection to fallback url
                        final_url = (char*)malloc((strlen(cgi_query->pageURL) + 1) * sizeof(char));
                        strcpy(final_url, cgi_query->pageURL);
                        redirection_status = 2;
                      }
                      else {
                        // finally, we have our url fetched and checked
                        final_url = (char*)malloc((strlen(row[0]) + 1) * sizeof(char));
                        strcpy(final_url, row[0]);
                        redirection_status = 1;
                      }
                    }
                  }
                  mysql_free_result(db_result);
                }
              }
              // send redirecition to final_url
              if (redirection_status != 0) {
                FCGX_PutS("Location: ", request.out);
                FCGX_PutS(final_url, request.out);
              }
              free(memc_fetched_url);
            }
          }

          FCGX_PutS("\n\n", request.out);
          FCGX_Finish_r(&request); // send answer to a user
                                   // and then finish work on this iteration
          if (redirection_status == 1) {
            memc_return_status = memcached_set(
                memc, cgi_query->username, strlen(cgi_query->username),
                final_url, strlen(final_url), (time_t)(memcached_conf.timeout), 0
            );
            if (memc_return_status != MEMCACHED_SUCCESS) {
              fprintf_threaded(log_mutex_ptr, log_file, "Coudn't store username+url in memcached\n");
              // not fatal error, continue working
            }
          }
          
          // Wait for db answer. Coudn't find how to cancel asyncronous mysql query.
          if (fetched_from_memcached) {
            while (db_async_status == NET_ASYNC_NOT_READY)
                db_async_status = mysql_real_query_nonblocking(
                    db_connection, db_query_string, (unsigned long)strlen(db_query_string)
                );
            MYSQL_RES *db_fake_result = mysql_store_result(db_connection);
            mysql_free_result(db_fake_result);
          }
          free(db_query_string);
          free_cgi_query(cgi_query);
          free(final_url);
        }

      }
      free(memcached_config_str);
      memcached_free(memc);
      FCGX_Free(&request, 0);
    }
    mysql_close(db_connection);
  }

  pthread_exit(NULL);
};

void fprintf_threaded(pthread_mutex_t *mut, FILE *file, char *msg) {
  pthread_mutex_lock(mut);
  fprintf (file, "%s\n", msg);
  pthread_mutex_unlock(mut);
};