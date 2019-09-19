#include "config_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "advanced_string.h"

int conf_parse_file(char *path, struct conf_node **buff) {
  FILE *config_file = fopen(path, "r");
  if (config_file != NULL) {
    char *config_data = (char*)malloc(sizeof(char));
    int conf_size = 1;
    while(!feof(config_file)) {
      conf_size++;
      config_data = (char*)realloc(config_data, conf_size * sizeof(char));
      fread(config_data + conf_size - 2, 1, 1, config_file);
    }
    config_data[conf_size - 1] = '\0';
    fclose(config_file);
    if (parenthesis_are_valid(config_data)) {
      int size = conf_parse_key_value(config_data, buff);
      free(config_data);
      return size;
    }
    free(config_data);
  }
  return -1;
}

int conf_parse_key_value(char *str, struct conf_node **buff) {
  *buff = (struct conf_node*) malloc(sizeof(struct conf_node));
  int buff_size = 0;
  char *key_iter = str,
       *val_iter = str;
  int buff_idx = -1;

  while (*key_iter != '\0' && *key_iter != '}') {
    buff_idx++;
    buff_size++;
    *buff = (struct conf_node*)realloc(*buff, buff_size * sizeof(struct conf_node));
    while (*key_iter == ' ')
      key_iter++;
    val_iter = key_iter;
    rewind_to(&val_iter, ' ');
    val_iter++;
    int key_len = len_before(key_iter, ' ') + 1;
    (*buff)[buff_idx].key = (char*)malloc(key_len * sizeof(char));
    copy_before((*buff)[buff_idx].key, key_iter, ' ');
    (*buff)[buff_idx].key[key_len - 1] = '\0';

    if (*val_iter == '{') {
      (*buff)[buff_idx].type = NODE;
      rewind_to(&key_iter, '\n');
      key_iter++;
      (*buff)[buff_idx].value_node_size = conf_parse_key_value(key_iter, &(*buff)[buff_idx].value);
      rewind_to(&key_iter, '}');
    }
    else {
      (*buff)[buff_idx].type = CHAR;
      int val_len = len_before(val_iter, '\n') + 1;
      (*buff)[buff_idx].value = malloc(val_len * sizeof(char));
      copy_before((*buff)[buff_idx].value, val_iter, '\n');
      *(char*)((*buff)[buff_idx].value + val_len - 1) = '\0';
    }

    rewind_to(&key_iter, '\n');
    key_iter++;
  }

  return buff_size;
}

void conf_free_node (struct conf_node *node, int size) {
  for (int i  = 0; i < size; i++) {
    free(node[i].key);
    if (node[i].type == NODE)
      conf_free_node((struct conf_node*)node[i].value, node[i].value_node_size);
    else
      free(node[i].value);
  }
  free(node);
}

int conf_find (char *key, struct conf_node *arr, int size) {
  for (int i = 0; i < size; i++) {
    if (!strcmp(arr[i].key, key))
      return i;
  }
  return -1;
}