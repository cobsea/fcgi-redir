#include "parsing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

void free_cgi_query(struct cgi_query *q) {
  if (q != NULL) {
    free(q->username);
    free(q->pageURL);
    free(q);
  }
};

/**
 * Copies symbols from one string to another until reaches end symbol
 */
void copy_before(char* cp_to, char *cp_from, char end) {
  char *copy_iter = cp_from,
       *paste_iter = cp_to;
  for (; *copy_iter != end && *copy_iter != '\0'; copy_iter++, paste_iter++)
    *paste_iter = *copy_iter;
};

/**
 * Calculates length before the next occurence of the end symbol
 */
int len_before(char *str, char end) {
  int ret = 0;
  for (char *i = str; *i != end && *i != '\0'; i++, ret++);
  return ret;
};

/**
 * Moves a given pointer to a given symbol
 */
void rewind_to(char **mutable_str_ptr, char sym) {
  for (; **mutable_str_ptr != sym && **mutable_str_ptr != '\0'; (*mutable_str_ptr)++);
};

/**
 * Compares 2 strings, but first string ends with f_end, second with s_end.
 */
int cmp_before(char *first, char f_end, char *second, char s_end) {
  char *f_ptr = first,
       *s_ptr = second;
  for (; (*f_ptr != f_end && *f_ptr != '\0') && (*s_ptr != s_end && *s_ptr != '\0');
          f_ptr++, s_ptr++) {
    if (*f_ptr > *s_ptr)
      return 1;
    else if (*f_ptr < *s_ptr)
      return -1;
  }
  if ((*f_ptr == f_end || *f_ptr == '\0') && (*s_ptr != s_end && *s_ptr != '\0'))
    return -1;
  if ((*f_ptr != f_end && *f_ptr != '\0') && (*s_ptr == s_end || *s_ptr == '\0'))
    return 1;

  return 0;
}

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

  return ret;
};

bool is_url(char *str) {
  char *iter = str;
  if (cmp_before(iter, ':', "http", '\0') != 0
  &&  cmp_before(iter, ':', "https", '\0') != 0)
    return false;

  rewind_to(&iter, ':');
  if (*iter == '\0' || *(iter + 1) == '\0')
    return false;

  iter++;
  if (*iter != '/' || *(iter + 1) != '/')
    return false;
  rewind_to(&iter, '.');
  if (*iter == '\0' || *(iter + 1) == '\0')
    return false;

  return true;
}