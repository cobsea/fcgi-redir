#ifndef FCGI_REDIR_FCGI_THREADS_H_
#define FCGI_REDIR_FCGI_THREADS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include <fcgi_config.h>
#include <fcgiapp.h>
#include <mysql.h>

#include "parsing.h"
#include "config_parser.h"
#include "meta_structs.h"

void *cgi_handler_thread(void *args);

void fprintf_threaded(pthread_mutex_t *mut, FILE *file, char *msg);

#endif