#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>

#include <fcgi_config.h>
#include <fcgiapp.h>
#include <mysql.h>

#include "parsing.h"
#include "config_parser.h"
#include "fcgi_threads.h"
#include "meta_structs.h"

bool finish_threads = false;

void exit_at_sigint(int signal) {
    finish_threads = true;
    // FCGX_ShutdownPending();
    fflush(NULL);
}

int main (int argc, char **argv) {
  // signal(SIGINT, exit_at_sigint);
  // TODO: config reading
  struct conf_node *config;
  int conf_size = conf_parse_file("config/test_app.conf", &config);

  int thread_count = 4; // change defaults!!
  char *socket_string = "127.0.0.1:9001";
  struct mysql_metadata mysql_conf;
  struct memcached_metadata memcached_conf;
  int val_idx = conf_find("thread_count", config, conf_size);
  if (val_idx >= 0 && config[val_idx].type == CHAR)
    thread_count = atoi((char*)(config[val_idx].value));

  val_idx = conf_find("fcgi_pass", config, conf_size);
  if (val_idx >= 0 && config[val_idx].type == CHAR) {
    socket_string = (char*)malloc((strlen((char*)(config[val_idx].value)) + 1) * sizeof(char));
    strcpy(socket_string, (char*)(config[val_idx].value));
  }

  val_idx = conf_find("mysql", config, conf_size);
  if (val_idx >= 0 && config[val_idx].type == NODE) {
    struct conf_node *ptr = (struct conf_node*)(config[val_idx].value);
    int inner_size = config[val_idx].value_node_size;
    int val_jdx = conf_find("host", ptr, inner_size);
    if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
      mysql_conf.host = (char*)malloc((strlen((char*)(ptr[val_jdx].value)) + 1) * sizeof(char));
      strcpy(mysql_conf.host, (char*)(ptr[val_jdx].value));
    }

    val_jdx = conf_find("port", ptr, inner_size);
    if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
      mysql_conf.port = (char*)malloc((strlen((char*)(ptr[val_jdx].value)) + 1) * sizeof(char));
      strcpy(mysql_conf.port, (char*)(ptr[val_jdx].value));
    }

    val_jdx = conf_find("user", ptr, inner_size);
    if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
      mysql_conf.user = (char*)malloc((strlen((char*)(ptr[val_jdx].value)) + 1) * sizeof(char));
      strcpy(mysql_conf.user, (char*)(ptr[val_jdx].value));
    }

    val_jdx = conf_find("password", ptr, inner_size);
    if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
      mysql_conf.password = (char*)malloc((strlen((char*)(ptr[val_jdx].value)) + 1) * sizeof(char));
      strcpy(mysql_conf.password, (char*)(ptr[val_jdx].value));
    }

    val_jdx = conf_find("database", ptr, inner_size);
    if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
      mysql_conf.database = (char*)malloc((strlen((char*)(ptr[val_jdx].value)) + 1) * sizeof(char));
      strcpy(mysql_conf.database, (char*)(ptr[val_jdx].value));
    }
  }

  val_idx = conf_find("memcached", config, conf_size);
  if (val_idx >= 0 && config[val_idx].type == NODE) {
    struct conf_node *ptr = (struct conf_node*)(config[val_idx].value);
    int inner_size = config[val_idx].value_node_size;
    int val_jdx = conf_find("host", ptr, inner_size);
    if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
      memcached_conf.host = (char*)malloc((strlen((char*)(ptr[val_jdx].value)) + 1) * sizeof(char));
      strcpy(memcached_conf.host, (char*)(ptr[val_jdx].value));
    }

    val_jdx = conf_find("port", ptr, inner_size);
    if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
      memcached_conf.port = (char*)malloc((strlen((char*)(ptr[val_jdx].value)) + 1) * sizeof(char));
      strcpy(memcached_conf.port, (char*)(ptr[val_jdx].value));
    }

    val_jdx = conf_find("timeout", ptr, inner_size);
    if (val_jdx >= 0 && ptr[val_jdx].type == CHAR) {
      memcached_conf.timeout = atoi((char*)(ptr[val_jdx].value));
    }
  }
  conf_free_node(config, conf_size);

  FILE *log_file = fopen("fcgi_redir_log.log", "a+");
  // end configuring

  if (mysql_library_init(0, NULL, NULL) != 0) {
    fprintf(log_file, "Coudn't init MYSQL library\n");
    // TODO: quit properly
  }

  if (FCGX_Init() != 0) {
    fprintf(log_file, "Coudn't init libfcgi\n");
    // TODO: quit properly
  }
  int socket_fd = FCGX_OpenSocket(socket_string, 1);
  if (socket_fd < 0) {
    fprintf(log_file, "Coudn't open socket\n");
    // TODO: quit properly
  }

  pthread_t *thread_ids = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);
  pthread_mutex_t request_mutex;
  pthread_mutex_init(&request_mutex, NULL);
  pthread_mutex_t log_mutex;
  pthread_mutex_init(&log_mutex, NULL);

  void **thread_args = (void**)malloc(sizeof(void*) * 6);
  thread_args[0] = (void*)&socket_fd;
  thread_args[1] = (void*)&mysql_conf;
  thread_args[2] = (void*)&memcached_conf;
  thread_args[3] = (void*)stdout;//log_file;
  thread_args[4] = (void*)&request_mutex;
  thread_args[5] = (void*)&log_mutex;

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

  // free resources
  mysql_library_end();
  fclose(log_file);
  free(thread_ids);
  free(thread_args);

  return 0;
}