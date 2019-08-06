#ifndef CGI_REDIR_PARSING_H_
#define CGI_REDIR_PARSING_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

struct cgi_query {
  char *username,
       *pageURL;
};

void free_cgi_query(struct cgi_query* q);
struct cgi_query *deduce_cgi_args(char *query);

bool is_url(char *str);

void copy_before(char* cp_to, char *cp_from, char end);
int len_before(char *str, char end);
void rewind_to(char **mutable_str_ptr, char sym);
int cmp_before(char *first, char f_end, char *second, char s_end);

#endif