#ifndef FCGI_REDIR_CONFIG_PARSER_H_
#define FCGI_REDIR_CONFIG_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "advanced_string.h"

enum conf_value_type{
  NODE,
  CHAR
};

struct conf_node {
  char *key;
  enum conf_value_type type;
  void *value;
  int value_node_size;
};

/**
 * Parses configuration file in such format:
 * 
 * key value
 * key {
 *  key value
 * }
 * 
 * and stores it in buff, which allocates automatically,
 * but you should free it with conf_free_node();
 * Returns size of buff.
 */
int conf_parse_file(char *path, struct conf_node **buff);

/**
 * Like conf_parse_file() but parses string
 */
int conf_parse_key_value(char *str, struct conf_node **buff);
void conf_free_node (struct conf_node *node, int size);

/**
 * Finds a key index in arr
 */
int conf_find (char *key, struct conf_node *arr, int size);

#endif