#pragma once
#include "defineds.h"
#include <llhttp.h>
#include <stdint.h>
#include <uv.h>

typedef struct route_s {
  char *static_url_path;
  int (*handler)(void);
} route_t;

typedef struct webconfig_s {
  char *www_root; /* local path */
  bool cors;      /* CORS */
  uint32_t def_cnt;
  char *defaults[]; /* default files */
} webconfig_t;

struct client_s;
typedef struct client_s client_t;

typedef struct request_s {
  char *url;
  uint32_t length_url;
  size_t length_path;
  char *path_content;
  size_t length_content;
  uint32_t try_default;
  client_t *client;
} request_t;

typedef struct client_s {
  uv_tcp_t handle;
  llhttp_t parser;

  uint32_t len_content_type;
  uv_fs_t req_open;
  uv_fs_t req_stat;
  uv_fs_t req_read;
  uv_fs_t req_close;
  uv_fs_t *req_sendfile;
  char response[512];
  bool sendfile;
  ssize_t filesize;

  uint32_t refCounter; /* for ref counter */
  request_t request;

  char *content_type;
  char *authorization;

  struct client_s *next; /* for utlist */
} client_t;

typedef struct mime_type_pair_s {
  const char *ext;
  const char *content_type;
} mime_type_pair_t;

int webserver(webconfig_t *config);
