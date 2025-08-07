#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "utarray.h"
#include "uv.h"

typedef struct response_s {
  char *path_content;
  size_t length_path;
  size_t size_content;
  const char *mime_content;
  uv_buf_t *buf;
  uv_file open_file;
} response_t;

typedef struct request_s {
  uint8_t method;
  char *url;
  uint32_t length_url;
  UT_array *query_param;

  char *body;
  size_t length_body;
  uint32_t default_filename_tries;
} request_t;

#define CLIENT_FLAG_IN_USE (1 << 0)
#define CLIENT_FLAG_IN_REF (1 << 1)
#define CLIENT_FLAG_MASK (CLIENT_FLAG_IN_USE | CLIENT_FLAG_IN_REF)

typedef struct client_s {
  uv_tcp_t handle;
  uint32_t flags : 2;   // Use bit 0 for in_use, bit 1 for in_ref
  request_t request;
  response_t response;
  struct client_s *next; /* for utlist */
  void *parser; // Opaque pointer to http_parser_t
} client_t;

client_t *client_create(uv_loop_t *loop);
void release_client(client_t *client);
void client_free(client_t *client);
void cleanup_resources(void);
void client_handle_request(client_t *client, const char *data, size_t len);


inline void client_set_in_use(client_t *client) {
  client->flags |= CLIENT_FLAG_IN_USE; // Set bit 0 for in_use
}

inline void client_clear_in_use(client_t *client) {
  client->flags &= ~CLIENT_FLAG_IN_USE; // Clear bit 0 for in_use
}

inline void client_set_in_ref(client_t *client) {
  client->flags |= CLIENT_FLAG_IN_REF; // Set bit 1 for in_ref
}

inline void client_clear_in_ref(client_t *client) {
  client->flags &= ~CLIENT_FLAG_IN_REF; // Clear bit 1 for in_ref
}

inline int client_is_in_use(const client_t *client) {
  return (client->flags & CLIENT_FLAG_IN_USE) != 0; // Check bit 0 for in_use
}

inline int client_is_in_ref(const client_t *client) {
  return (client->flags & CLIENT_FLAG_IN_REF) != 0; // Check bit 1 for in_ref
}

inline int client_is_flags_free(const client_t *client) {
  return (client->flags & CLIENT_FLAG_MASK) == 0; // Check if both flags are clear
}
