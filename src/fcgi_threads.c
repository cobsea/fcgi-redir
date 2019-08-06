#include "fcgi_threads.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include <fcgi_config.h>
#include <fcgiapp.h>
#include <mysql.h>
#include <libmemcached/memcached.h>

#include "config_parser.h"
#include "parsing.h"
#include "meta_structs.h"

void *cgi_handler_thread(void *args) {
  // deducing arguments
  int socket_fd = *((int*)(((void**)args)[0]));
  struct mysql_metadata mysql = *((struct mysql_metadata*)(((void**)args)[1]));
  struct memcached_metadata memcached_conf = *((struct memcached_metadata*)(((void**)args)[2]));
  FILE *log_file = (FILE*)(((void**)args)[3]);
  pthread_mutex_t *request_mutex_ptr = (pthread_mutex_t*)(((void**)args)[4]);
  pthread_mutex_t *log_mutex_ptr = (((void**)args)[5]);

  // connecting to MySQL
  MYSQL *db_connection = mysql_init(NULL);
  if (db_connection == NULL) {
    fprintf_threaded(log_mutex_ptr, log_file, "Coudn't init mysql in thread\n");
    // TODO: exit thread properly
  }
  if (mysql_real_connect(db_connection, mysql.host, mysql.user, mysql.password, mysql.database, 0, NULL, 0) == NULL) {
    fprintf_threaded(log_mutex_ptr, log_file, "Coudn't connect to database\n");
    // TODO: exit thread properly
  }

  // connecting to memcached daemon
  char *memcached_config_str = (char*)malloc((strlen(memcached_conf.host) + /*port +*/ 10) * sizeof(char));
  sprintf(memcached_config_str, "--SERVER=%s", memcached_conf.host);
  memcached_st *memc = memcached(memcached_config_str, strlen(memcached_config_str));
  if (memc == NULL) {
    fprintf_threaded(log_mutex_ptr, log_file, "Coudn't connect to memcached\n");
    // not so fatal error, so continue working with db-only
  }

  // initializing FastCGI
  FCGX_Request request;
  if (FCGX_InitRequest(&request, socket_fd, 0) != 0) {
    fprintf_threaded(log_mutex_ptr, log_file, "Coudn't initiate request\n");
    // TODO: exit thread properly
  }

  while (1) {
    pthread_mutex_lock(request_mutex_ptr);
    if (FCGX_Accept_r(&request) < 0)
      fprintf_threaded(log_mutex_ptr, log_file, "Coudn't accept request\n");
    pthread_mutex_unlock(request_mutex_ptr);

    char *cgi_query_string = FCGX_GetParam("QUERY_STRING", request.envp);
    struct cgi_query *cgi_query = deduce_cgi_args(cgi_query_string);

    FCGX_PutS("Content-Type: text/html\n"
              "Connection: close\n", request.out);

    if (cgi_query == NULL) {
      FCGX_PutS("Status: 502 Incorrect Query\n", request.out);
      FCGX_PutS("\n<!DOCTYPE html><html><head><title>502 Incorrect query</title></head><body>"
                "<center><h1>Error 502: Incorrect query.</h1></center><br>\n"
                "<h3>Proper URL looks like this:</h3><br>\n"
                "<h3>http://1.2.3.4/app/fcgi_redir?username=Anil&ran=0.97584378943&pageURL=http://xyz.com</h3>"
                "</body></html>", request.out);
    }
    else {
      char *db_query_string = (char*)malloc((37 + strlen(cgi_query->username)) * sizeof(char));
      sprintf(db_query_string, "SELECT url FROM pairs WHERE name='%s'", cgi_query->username);

      // the plan:
      // async db query
      // sync memcached query
      // if memcached query successed, cancel db query
      // otherwise fetch db answer

      if (mysql_query(db_connection, db_query_string) != 0) {
        fprintf_threaded(log_mutex_ptr, log_file, "Coudn't send mysql query\n");
        FCGX_PutS("\n<html><head><title>Error</title></head><body>Can't connect to db</body></html>", request.out);
      }
      else {
        MYSQL_RES *db_result = mysql_store_result(db_connection);
        if (db_result == NULL) {
          fprintf_threaded(log_mutex_ptr, log_file, "Coudn't receive mysql result\n");
          FCGX_PutS("\n<html><head><title>Error</title></head><body>Can't store db result</body></html>", request.out);
        }
        else {
          FCGX_PutS("Location: ", request.out);
          MYSQL_ROW row = mysql_fetch_row(db_result);
          if (row == NULL) {
            FCGX_PutS(cgi_query->pageURL, request.out);
          }
          else {
            if (!is_url(row[0])) {
              FCGX_PutS(cgi_query->pageURL, request.out);
            }
            else {
              FCGX_PutS(row[0], request.out);
            }
          }
          mysql_free_result(db_result);
        }
      }
      free(db_query_string);
    }

    FCGX_PutS("\n\n", request.out);
    FCGX_Finish_r(&request);
    free_cgi_query(cgi_query);
  }

  memcached_free(memc);
  FCGX_Free(&request, 0);
  mysql_close(db_connection);

  pthread_exit(NULL);
};

void fprintf_threaded(pthread_mutex_t *mut, FILE *file, char *msg) {
  pthread_mutex_lock(mut);
  fprintf (file, "%s\n", msg);
  pthread_mutex_unlock(mut);
};