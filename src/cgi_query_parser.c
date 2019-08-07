#include "cgi_query_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "advanced_string.h"

void free_cgi_query(struct cgi_query *q) {
  if (q != NULL) {
    free(q->username);
    free(q->pageURL);
    free(q);
  }
};

struct cgi_query *deduce_cgi_args(char *query) {
  struct cgi_query *ret = (struct cgi_query*)malloc(sizeof(struct cgi_query));
  char *q_iter = query;
  // check username:
  if (cmp_before(q_iter, '=', "username", '\0') != 0) {
    free(ret);
    return NULL;
  }
  rewind_to(&q_iter, '=');
  if (*q_iter == '\0' || *(q_iter + 1) == '\0') {
    free(ret);
    return NULL;
  }
  q_iter++;
  int uname_size = len_before(q_iter, '&');
  ret->username = (char*)malloc((uname_size + 1) * sizeof(char));
  copy_before(ret->username, q_iter, '&');
  ret->username[uname_size] = '\0';

  // check ran:
  rewind_to(&q_iter, '&');
  if (*q_iter == '\0' || *(q_iter + 1) == '\0') {
    free(ret->username);
    free(ret);
    return NULL;
  }
  q_iter++;
  if (cmp_before(q_iter, '=', "ran", '\0') != 0) {
    free(ret->username);
    free(ret);
    return NULL;
  }
  q_iter++;

  // check pageURL
  rewind_to(&q_iter, '&');
  if (*q_iter == '\0' || *(q_iter + 1) == '\0') {
    free(ret->username);
    free(ret);
    return NULL;
  }
  q_iter++;
  if (cmp_before(q_iter, '=', "pageURL", '\0') != 0) {
    free(ret->username);
    free(ret);
    return NULL;
  }
  rewind_to(&q_iter, '=');
  if (*q_iter == '\0' || *(q_iter + 1) == '\0') {
    free(ret->username);
    free(ret);
    return NULL;
  }
  q_iter++;
  int url_size = len_before(q_iter, '\0');
  ret->pageURL = (char*)malloc((url_size + 1) * sizeof(char));
  copy_before(ret->pageURL, q_iter, '\0');
  ret->pageURL[url_size] = '\0';

  return ret;
};