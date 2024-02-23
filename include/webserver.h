#pragma once
#include "defineds.h"
#include <llhttp.h>
#include <stdint.h>
#include <utarray.h>
#include <utlist.h>
#include <uv.h>

typedef struct route_s {
  char *static_url_path;
  int (*handler)(void);
} route_t;

typedef struct webconfig_s {
  char *host;
  uint16_t port;
  char *www_root; /* local path */
  bool cors;      /* CORS */
  uint32_t def_cnt;
  char *defaults[]; /* default files */
} webconfig_t;

struct client_s;
typedef struct client_s client_t;

typedef struct get_param_s {
  char *name;
  char *value;
} get_param_t;

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

typedef struct client_s {
  uv_tcp_t handle;
  llhttp_t parser;
  // Use bit 0 for in_use, bit 1 for in_ref
  uint32_t flags : 2;

  request_t request;
  response_t response;

  struct client_s *next; /* for utlist */
} client_t;

#define CLIENT_FLAG_IN_USE (1 << 0)
#define CLIENT_FLAG_IN_REF (1 << 1)
#define CLIENT_FLAG_MASK (CLIENT_FLAG_IN_USE | CLIENT_FLAG_IN_REF)

// Macro to check the in_use flag
#define CLIENT_IS_IN_USE(client) ((client)->flags & CLIENT_FLAG_IN_USE)
// Macro to set the in_use flag
#define CLIENT_SET_IN_USE(client) ((client)->flags |= CLIENT_FLAG_IN_USE)
// Macro to clear the in_use flag
#define CLIENT_CLEAR_IN_USE(client) ((client)->flags &= ~CLIENT_FLAG_IN_USE)
// Macro to check the in_ref flag
#define CLIENT_IS_IN_REF(client) ((client)->flags & CLIENT_FLAG_IN_REF)
// Macro to set the in_ref flag
#define CLIENT_SET_IN_REF(client) ((client)->flags |= CLIENT_FLAG_IN_REF)
// Macro to clear the in_ref flag
#define CLIENT_CLEAR_IN_REF(client) ((client)->flags &= ~CLIENT_FLAG_IN_REF)
// Macro to check flags
#define CLIENT_IS_FLAGS_FREE(client)                                           \
  (((client)->flags & (CLIENT_FLAG_IN_USE | CLIENT_FLAG_IN_REF)) == 0)

typedef struct mime_type_pair_s {
  const char *ext;
  const char *content_type;
} mime_type_pair_t;

typedef uint32_t statuscode_t;
typedef struct {
  statuscode_t code;
  const char *reason_phrase;
} http_status_code_t;

typedef struct header_entry_s {
  char *field;
  char *value;
  struct header_entry_s *next;
} header_entry_t;

/**
 * @brief Starts the web server.
 *
 * This function initializes the libuv event loop, sets up HTTP parser settings,
 * initializes a TCP server, binds it to the specified address and port,
 * listens for incoming connections, and starts the event loop to handle client
 * requests.
 *
 * @param ev_loop Pointer to a uv_loop_t structure
 * @param config Pointer to the web server configuration structure.
 *               If NULL, default configuration will be used.
 *
 * @return Returns 0 upon successful execution, or a non-zero value if an error
 *         occurs.
 */
int webserver(uv_loop_t *ev_loop, webconfig_t *config);
