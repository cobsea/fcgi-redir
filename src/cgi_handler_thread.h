#ifndef FCGI_REDIR_CGI_HANDLER_THREAD_H_
#define FCGI_REDIR_CGI_HANDLER_THREAD_H_

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

void *cgi_handler_thread(void *args);

void fprintf_threaded(pthread_mutex_t *mut, FILE *file, char *msg);

#endif