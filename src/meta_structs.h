#ifndef FCGI_REDIR_META_STRUCTS_H_
#define FCGI_REDIR_META_STRUCTS_H_

struct memcached_metadata {
  char *host,
       *port;
  int timeout;
};

struct mysql_metadata {
  char *user,
       *host,
       *port,
       *password,
       *database;
};

#endif