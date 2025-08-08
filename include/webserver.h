#pragma once
#include "defineds.h"
#include <llhttp.h>
#include "http_parser.h"
#include <stdint.h>
#include <utarray.h>
#include <utlist.h>
#include <uv.h>

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
