#ifndef FCGI_REDIR_ADVANCED_STRING_H_
#define FCGI_REDIR_ADVANCED_STRING_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

/**
 * Copies symbols from one string to another until reaches end symbol
 */
void copy_before(char* cp_to, char *cp_from, char end);

/**
 * Calculates length before the next occurence of the end symbol
 */
int len_before(char *str, char end);

/**
 * Moves a given pointer to a given symbol
 */
void rewind_to(char **mutable_str_ptr, char sym);

/**
 * Compares 2 strings, but first string ends with f_end, second with s_end.
 */
int cmp_before(char *first, char f_end, char *second, char s_end);

/**
 * Checks if a given string is URL
 */
bool is_url(char *str);

#endif