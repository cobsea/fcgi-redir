#include "advanced_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

void copy_before(char* cp_to, char *cp_from, char end) {
  char *copy_iter = cp_from,
       *paste_iter = cp_to;
  for (; *copy_iter != end && *copy_iter != '\0'; copy_iter++, paste_iter++)
    *paste_iter = *copy_iter;
};

int len_before(char *str, char end) {
  int ret = 0;
  for (char *i = str; *i != end && *i != '\0'; i++, ret++);
  return ret;
};

void rewind_to(char **mutable_str_ptr, char sym) {
  for (; **mutable_str_ptr != sym && **mutable_str_ptr != '\0'; (*mutable_str_ptr)++);
};

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