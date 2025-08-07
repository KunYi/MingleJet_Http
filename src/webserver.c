
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* include libuv & llhttp */
#include "client_manager.h"
#include "default_response.h"
#include "defineds.h"
#include "file_handle.h"
#include "router.h"
#include "utils.h"
#include "webserver.h"

static uv_loop_t *loop;
static uv_signal_t sigint_handle, sigterm_handle;
static uv_timer_t release_timer;

// static void setup_cleanup_timer(uv_loop_t *loop) {
//   // Release resource1 after 200ms
//   uv_timer_init(loop, &release_timer);
//   uv_timer_start(&release_timer, (uv_timer_cb)&cleanup_freeList, 200, 200);
// }

/* -------------------------------------------------------------------------------------------
 */
static void on_alloc(uv_handle_t *handle, size_t suggested_size,
                     uv_buf_t *buf) {
  UNUSED(handle);
  *buf = uv_buf_init((char *)malloc(suggested_size), suggested_size);
}

static void on_close(uv_handle_t *handle) {
  client_t *client = (client_t *)(handle->data);
  release_client(client);
}

// Callback to handle HTTP request data
void on_request_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  const uv_tcp_t *handle = (uv_tcp_t *)stream;
  client_t *client = (client_t *)(handle->data);

  if (nread < 0) { // Error or EOF
    if (nread != UV_EOF) {
      fprintf(stderr, "Read error %s\n", uv_strerror(nread));
    }
    fprintf(stdout, "UV_EOF, close the connection\n\n");
    uv_close((uv_handle_t *)&client->handle, (uv_close_cb)on_close);
    free(buf->base);
    return;
  }

  if (nread == 0) {
    uv_close((uv_handle_t *)&client->handle, (uv_close_cb)on_close);
    return;
  }

  llhttp_t *parser = &((http_parser_t *)client->parser)->parser;
  // Parse the received data
  enum llhttp_errno err = llhttp_execute(parser, buf->base, nread);
  if (err != HPE_OK) {
    fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(err),
            ((http_parser_t *)client->parser)->parser.reason);
    uv_close((uv_handle_t *)&client->handle, (uv_close_cb)on_close);
    free(buf->base);
    return;
  }

  client->request.method = parser->method;
  // parsed successfully
  router_dispatch(client);
  free(buf->base);
}

/**
 * @brief Callback function for new client connections.
 *
 * This function is invoked when a new client connection is established on the
 * server. It initializes the client structure, sets up the libuv handle, and
 * starts reading data from the client. If an error occurs during connection
 * establishment, it prints an error message.
 *
 * @param server  Pointer to the uv_stream_t structure representing the server.
 * @param status  Connection status.
 */
static void on_connection(uv_stream_t *server, int status) {
  if (status < 0) {
    fprintf(stderr, "New connection error %s\n", uv_strerror(status));
    return;
  }

  client_t *client = client_create(loop);
  if (uv_accept(server, (uv_stream_t *)client) == 0) {
    uv_read_start((uv_stream_t *)&(client->handle), on_alloc, on_request_read);
  } else {
    uv_close((uv_handle_t *)&(client->handle), (uv_close_cb)on_close);
    fprintf(stderr, "New connection error %s\n", uv_strerror(status));
  }
}

static void walk_cb(uv_handle_t *handle, void *arg) {
  UNUSED(arg);
  if (!uv_is_closing(handle)) {
    uv_close(handle, NULL);
  }
}

static void signal_handler(uv_signal_t *handle, int signum) {
  UNUSED(signum);
  uv_stop(loop);
}

static void showLibrariesInfo(void) {
  fprintf(stdout, "use the below third party components\n");
  // Print third-party component versions
  fprintf(stdout, "  libuv:%d.%d.%d %s\n", UV_VERSION_MAJOR, UV_VERSION_MINOR,
          UV_VERSION_PATCH, UV_VERSION_IS_RELEASE ? "Release" : "Testing");
  fprintf(stdout, "  llhttp:%d.%d.%d\n", LLHTTP_VERSION_MAJOR,
          LLHTTP_VERSION_MINOR, LLHTTP_VERSION_PATCH);
  fprintf(stdout, "  uthash:%s (for utlist/utarray)\n",
          STR_VERSION(UTLIST_VERSION));
}

int webserver(uv_loop_t *ev_loop, webconfig_t *config) {
  if (ev_loop == NULL)
    return -1;

  loop = ev_loop;
  webconfig_t *web_config = NULL;

  // Set configuration if provided
  if (config != NULL) {
    web_config = config;
  } else {
    // Use default configuration
    // (optional: set default configuration)
  }

  file_handle_init(loop, web_config);
  router_init();

  // Initialize signal handlers
  uv_signal_init(loop, &sigint_handle);
  uv_signal_init(loop, &sigterm_handle);

  // Register signal handlers
  uv_signal_start(&sigint_handle, signal_handler, SIGINT);
  uv_signal_start(&sigterm_handle, signal_handler, SIGTERM);

  // Initialize TCP server
  uv_tcp_t server;
  uv_tcp_init(loop, &server);

  // Bind server to specified address and port
  struct sockaddr_in bind_addr;
  uv_ip4_addr(web_config->host, web_config->port, &bind_addr);
  uv_tcp_bind(&server, (const struct sockaddr *)&bind_addr, 0);

  // Start listening for incoming connections
  int r = uv_listen((uv_stream_t *)&server, SOMAXCONN, on_connection);
  if (r) {
    fprintf(stderr, "Listen error %s\n", uv_strerror(r));
    return -1;
  }

  // Print server information
  fprintf(stdout, "Launch MingleJet...\n\n");
  showLibrariesInfo();
  fprintf(stdout, "\n");
  // Print server listening information
  fprintf(stdout, "Server listening on port %d...\n\n", web_config->port);

  // Setup timer for cleanup
  // setup_cleanup_timer(loop);

  // Run libuv event loop
  int ret = uv_run(loop, UV_RUN_DEFAULT);

  uv_timer_stop(&release_timer);
  uv_close((uv_handle_t *)&release_timer, NULL);

  // Release resources
  uv_walk(loop, walk_cb, 0);
  uv_run(loop, UV_RUN_DEFAULT); // Run pending callbacks

  // Clean up resources and close event loop
  cleanup_resources();

  // the following will release in uv_walk()
  // uv_signal_stop(&sigint_handle);
  // uv_signal_stop(&sigterm_handle);

  fprintf(stdout, "Server Shutdown now\n");
  return ret;
}
