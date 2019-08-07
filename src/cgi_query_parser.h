#ifndef FCGI_REDIR_CGI_QUERY_PARSER_H_
#define FCGI_REDIR_CGI_QUERY_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "advanced_string.h"

struct cgi_query {
  char *username,
       *pageURL;
};

struct cgi_query *deduce_cgi_args(char *query);
void free_cgi_query(struct cgi_query* q);

#endif