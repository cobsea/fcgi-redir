#ifndef FCGI_REDIR_CONFIG_PARSER_H_
#define FCGI_REDIR_CONFIG_PARSER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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

int conf_parse_file(char *path, struct conf_node **buff);
int conf_parse_key_value(char *str, struct conf_node **buff);
void conf_free_node (struct conf_node *node, int size);

// returns index
int conf_find (char *key, struct conf_node *arr, int size);

#endif