#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcgi_config.h>
#include <fcgiapp.h>
#include <mysql.h>

#include "cgi_handler_thread.h"
#include "config_parser.h"
#include "meta_structs.h"

bool finish_threads = false;

void exit_at_sigterm(int signal) {
    finish_threads = true;
    FCGX_ShutdownPending();
    fflush(NULL);
}

int main (int argc, char **argv) {
  signal(SIGTERM, exit_at_sigterm);
  signal(SIGINT, exit_at_sigterm);

  // setting up default values for configuration:
  int thread_count = 4;
  char *socket_string = (char*)malloc(15 * sizeof(char));
  strcpy(socket_string, "127.0.0.1:9000");

  struct mysql_metadata mysql_conf;
  mysql_conf.user = (char*)malloc(5 * sizeof(char));
  strcpy(mysql_conf.user, "root");
  mysql_conf.password = (char*)malloc(5 * sizeof(char));
  strcpy(mysql_conf.password, "toor");
  mysql_conf.host = (char*)malloc(10 * sizeof(char));
  strcpy(mysql_conf.host, "127.0.0.1");
  mysql_conf.port = 3306;
  mysql_conf.database = (char*)malloc(16 * sizeof(char));
  strcpy(mysql_conf.database, "fcgi_redir_data");

  struct memcached_metadata memcached_conf;
  memcached_conf.host = (char*)malloc(10 * sizeof(char));
  strcpy(memcached_conf.host, "127.0.0.1");
  memcached_conf.port = (char*)malloc(6 * sizeof(char));
  strcpy(memcached_conf.port, "11211");
  memcached_conf.timeout = 10;

  // configuring with file
  struct conf_node *config;
  int conf_size = conf_parse_file("config/fcgi_redir.conf", &config);
  if (conf_size > 0) {
    int val_idx = conf_find("thread_count", config, conf_size);
    if (val_idx >= 0 && config[val_idx].type == CHAR)
      thread_count = atoi((char*)(config[val_idx].value));

    val_idx = conf_find("fcgi_pass", config, conf_size);
    if (val_idx >= 0 && config[val_idx].type == CHAR) {
      socket_string = (char*)realloc(socket_string, (strlen((char*)(config[val_idx].value)) + 1) * sizeof(char));
      strcpy(socket_string, (char*)(config[val_idx].value));
    }

    val_idx = conf_find("mysql", config, conf_size);
    if (val_idx >= 0 && config[val_idx].type == NODE) {
      struct conf_node *ptr = (struct conf_node*)(config[val_idx].value);
      int inner_size = config[val_idx].value_node_size;
      int val_jdx = conf_find("host", ptr, inner_size);
      if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
        mysql_conf.host = (char*)realloc(mysql_conf.host, (strlen((char*)(ptr[val_jdx].value)) + 1) * sizeof(char));
        strcpy(mysql_conf.host, (char*)(ptr[val_jdx].value));
      }

      val_jdx = conf_find("port", ptr, inner_size);
      if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
        mysql_conf.port = atoi((char*)(ptr[val_jdx].value));
      }

      val_jdx = conf_find("user", ptr, inner_size);
      if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
        mysql_conf.user = (char*)realloc(mysql_conf.user, (strlen((char*)(ptr[val_jdx].value)) + 1) * sizeof(char));
        strcpy(mysql_conf.user, (char*)(ptr[val_jdx].value));
      }

      val_jdx = conf_find("password", ptr, inner_size);
      if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
        mysql_conf.password = (char*)realloc(mysql_conf.password, (strlen((char*)(ptr[val_jdx].value)) + 1) * sizeof(char));
        strcpy(mysql_conf.password, (char*)(ptr[val_jdx].value));
      }

      val_jdx = conf_find("database", ptr, inner_size);
      if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
        mysql_conf.database = (char*)realloc(mysql_conf.database, (strlen((char*)(ptr[val_jdx].value)) + 1) * sizeof(char));
        strcpy(mysql_conf.database, (char*)(ptr[val_jdx].value));
      }
    }

    val_idx = conf_find("memcached", config, conf_size);
    if (val_idx >= 0 && config[val_idx].type == NODE) {
      struct conf_node *ptr = (struct conf_node*)(config[val_idx].value);
      int inner_size = config[val_idx].value_node_size;
      int val_jdx = conf_find("host", ptr, inner_size);
      if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
        memcached_conf.host = (char*)realloc(memcached_conf.host, (strlen((char*)(ptr[val_jdx].value)) + 1) * sizeof(char));
        strcpy(memcached_conf.host, (char*)(ptr[val_jdx].value));
      }

      val_jdx = conf_find("port", ptr, inner_size);
      if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
        memcached_conf.port = (char*)realloc(memcached_conf.port, (strlen((char*)(ptr[val_jdx].value)) + 1) * sizeof(char));
        strcpy(memcached_conf.port, (char*)(ptr[val_jdx].value));
      }

      val_jdx = conf_find("timeout", ptr, inner_size);
      if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
        memcached_conf.timeout = atoi((char*)(ptr[val_jdx].value));
      }
    }
  }
  else {
    printf("Parenthesis issue!\n");
  }
  conf_free_node(config, conf_size);
  // configuration is finished

  FILE *log_file = fopen("fcgi_redir_log.log", "a+");

  if (mysql_library_init(0, NULL, NULL) != 0) {
    fprintf(log_file, "Coudn't init MYSQL library\n");
  }
  else {
    if (FCGX_Init() != 0) {
      fprintf(log_file, "Coudn't init libfcgi\n");
    }
    else {
      int socket_fd = FCGX_OpenSocket(socket_string, 1);
      if (socket_fd < 0) {
        fprintf(log_file, "Coudn't open socket\n");
      }
      else {
        pthread_t *thread_ids = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);
        pthread_mutex_t request_mutex;
        pthread_mutex_init(&request_mutex, NULL);
        pthread_mutex_t log_mutex;
        pthread_mutex_init(&log_mutex, NULL);

        void **thread_args = (void**)malloc(sizeof(void*) * 7);
        thread_args[0] = (void*)&socket_fd;
        thread_args[1] = (void*)&mysql_conf;
        thread_args[2] = (void*)&memcached_conf;
        thread_args[3] = (void*)log_file;
        thread_args[4] = (void*)&request_mutex;
        thread_args[5] = (void*)&log_mutex;
        thread_args[6] = (void*)&finish_threads;

        for (int i = 0; i < thread_count; i++) {
          int error_code = pthread_create(thread_ids + i, NULL, cgi_handler_thread, thread_args);
          if (error_code != 0) {
            pthread_mutex_lock(&log_mutex);
            fprintf (log_file, "Something went wrong while creating thread #%d. Error code: %d.\n", i, error_code);
            pthread_mutex_unlock(&log_mutex);
          };
        }

        for (int i = 0; i < thread_count; i++)
          pthread_join(thread_ids[i], NULL);

        free(thread_ids);
        free(thread_args);
      }
    }
  }

  // free resources
  mysql_library_end();
  free(mysql_conf.user);
  free(mysql_conf.host);
  free(mysql_conf.password);
  free(mysql_conf.database);
  free(memcached_conf.host);
  free(memcached_conf.port);
  fclose(log_file);

  return 0;
}