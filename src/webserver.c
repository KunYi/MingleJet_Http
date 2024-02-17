
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* include libuv & llhttp */
#include "defineds.h"
#include "webserver.h"
#include <llhttp.h>
#include <utlist.h>
#include <uv.h>

#define DEFAULT_PORT 8080

static const char *response200 = "HTTP/1.1 200 OK\r\n"
                                 "Content-Type: %s\r\n"
                                 "Content-Length: %d\r\n"
                                 "\r\n";

static const char *response404 = "HTTP/1.1 404 Not Found\r\n"
                                 "Content-Type: text/html\r\n"
                                 "Content-Length: 85\r\n"
                                 "\r\n"
                                 "<html>"
                                 "<header>"
                                 "<title>MingleJet</title>"
                                 "</header>"
                                 "<body>"
                                 "<H1>Not Found</H1>"
                                 "</body>"
                                 "</html>";

static const char *response401 = "HTTP/1.1 401 Unauthorized\r\n"
                                 "Location: %s\r\n"
                                 "Content-Length: 0\r\n"
                                 "\r\n";

// static mime_type_pair_t mime_types[] = {
//   { ".html", "text/html" },
//   { ".htm", "text/html" },
//   { ".css", "text/css" },
//   { ".js",  "text/javascript" },
//   { ".mjs", "text/javascript" },
//   { ".json", "application/json" },
//   { ".txt", "text/plain" },
//   { ".png", "image/png" },
//   { "svg", "image/svg+xml" },
//   { ".jpg", "image/jpeg" },
//   { ".jpeg", "image/jpeg" },
//   { ".gif", "image/gif" },
//   { ".ico", "image/vnd.microsoft.icon"},
//   { ".ttf", "font/ttf" },
//   { ".woff", "font/woff" },
//   { ".woff2", "font/woff2" },
//   { ".pdf", "application/pdf" },
//   { "", "application/octet-stream" },
// };

static mime_type_pair_t mime_types[] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "text/javascript"},
    {".mjs", "text/javascript"},
    {".json", "application/json"},
    {".txt", "text/plain"},
    {".png", "image/png"},
    {".svg", "image/svg+xml"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".ico", "image/vnd.microsoft.icon"},
    {".ttf", "font/ttf"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".pdf", "application/pdf"},
    {".mp3", "audio/mpeg"},
    {".ogg", "audio/ogg"},
    {".wav", "audio/wav"},
    // { ".mp4", "video/mp4" },
    // { ".mov", "video/quicktime" },
    // { ".avi", "video/x-msvideo" },
    {".zip", "application/zip"},
    {".gz", "application/gzip"},
    {".rar", "application/vnd.rar"},
    // { ".doc", "application/msword" },
    // { ".docx",
    // "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
    // }, { ".xlsx",
    // "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" }, {
    // ".pptx",
    // "application/vnd.openxmlformats-officedocument.presentationml.presentation"
    // }, { ".eml", "message/rfc822" },
    {".bmp", "image/bmp"},
    {".tiff", "image/tiff"},
    {".XXXYYY", "application/octet-stream"},
};

static webconfig_t *web_config;
static uv_loop_t *loop;
static uv_signal_t sigint_handle, sigterm_handle;
static uv_idle_t *idle_handle = NULL;

// HTTP parser settings
llhttp_settings_t settings;

// HTTP request structure

static void on_write(uv_write_t *req, int status);

// Buffer to store received data
char buffer[4096];

client_t *activeClientList = NULL;
client_t *freeClientList = NULL;

// 清理待释放队列的回调函数
static void cleanup_freeList(uv_idle_t *handle) {
  UNUSED(handle);
  // 执行清理操作
  client_t *elt, *tmp;
  LL_FOREACH_SAFE(freeClientList, elt, tmp) {
    if (elt->refCounter == 0) {
      LL_DELETE(freeClientList, elt);
      if (elt->request.url != NULL)
        free(elt->request.url);
      free(elt);
    }
  }
}

static void cleanup_resources(void) {
  client_t *elt, *tmp;
  LL_FOREACH_SAFE(freeClientList, elt, tmp) {
    LL_DELETE(freeClientList, elt);
    if (elt->request.url != (char *)NULL) {
      free(elt->request.url);
    }
    free(elt);
  }

  LL_FOREACH_SAFE(activeClientList, elt, tmp) {
    LL_DELETE(activeClientList, elt);
    if (elt->request.url != (char *)NULL) {
      free(elt->request.url);
    }
    free(elt);
  }
}

// 在事件循环中设置空闲处理器，空闲时执行清理操作
void setup_cleanup_idle(uv_loop_t *loop) {
  // 创建一个 uv_idle_t 结构体
  idle_handle = (uv_idle_t *)malloc(sizeof(uv_idle_t));

  // 初始化空闲处理器，并设置回调函数
  uv_idle_init(loop, idle_handle);
  uv_idle_start(idle_handle, cleanup_freeList);
}

/* -------------------------------------------------------------------------------------------
 */

static const char *match_mime_type(const char *path) {
  const char *ext = strrchr(path, '.');
  if (!ext) {
    // Handle unknown extension cases (set default or error)
    return "application/octet-stream";
  }

  const char *content_type = "application/octet-stream";

  for (size_t i = 0; i < sizeof(mime_types) / sizeof(mime_types[0]); i++) {
    if (strcasecmp(ext, mime_types[i].ext) == 0) {
      content_type = mime_types[i].content_type;
      break;
    }
  }
  return content_type;
}

static void final_sendfile(uv_fs_t *req) {
  client_t *client = (client_t *)req->data;

  uv_fs_t req_close;
  uv_fs_close(loop, &req_close, client->req_open.result, NULL);
  uv_fs_req_cleanup(req);
  free(req);
  client->refCounter--;
}

static void send_file_context(uv_fs_t *fs_req) {
  request_t *req = (request_t *)fs_req->data;
  client_t *client = req->client;

  // FIXME: only for linux
  if (fs_req->result >= 0) {
    uv_os_fd_t sendfd;
    uv_fs_t *send_req = (uv_fs_t *)malloc(sizeof(uv_fs_t));
    send_req->data = client;
    uv_fileno((uv_handle_t *)&client->handle, &sendfd);
    client->refCounter++;
    uv_fs_sendfile(loop, send_req, sendfd, fs_req->result, 0,
                   req->length_content, final_sendfile);
#ifdef _WIN32
#error "because windows not support sendfile(), need implement"
#endif
  }

  // release path
  free(req->path_content);
  req->path_content = NULL;

  uv_fs_req_cleanup(fs_req);
  free(fs_req);
}

static void open_send_file(uv_write_t *req, int status) {
  if (status == 0) {
    request_t *request = (request_t *)req->data;
    uv_fs_t *fs_req = malloc(sizeof(uv_fs_t));
    fs_req->data = request;
    uv_fs_open(loop, fs_req, request->path_content, O_RDONLY,
               (S_IRUSR | S_IRGRP), send_file_context);
  }
  free(req);
}

static void found_and_sendfs_req(request_t *req) {
  client_t *client = req->client;
  char buf[128];
  snprintf(buf, sizeof(buf), response200, match_mime_type, req->length_content);
  uv_buf_t resbuf = uv_buf_init((char *)buf, strlen(buf));

  // send header of response;
  uv_write_t *write_req = malloc(sizeof(uv_write_t));
  write_req->data = (void *)req;
  uv_write(write_req, (uv_stream_t *)&client->handle, &resbuf, 1,
           open_send_file);
}

static void check_default_files_async(uv_fs_t *fs_req) {
  request_t *req = (request_t *)fs_req->data;
  client_t *client = req->client;

  if (fs_req->result != 0) {
    // fprintf(stdout, "Can't find file: %s\n", fs_req->path);
    req->try_default++;
    if (req->try_default >= web_config->def_cnt) {
      // fprintf(stdout, "Can't find any default file\n");
      // FIXME: response 404
      uv_buf_t resbuf = uv_buf_init((char *)response404, strlen(response404));
      // send reponse;
      uv_write_t *write_req = malloc(sizeof(uv_write_t));
      write_req->data = (void *)client;
      uv_write(write_req, (uv_stream_t *)&client->handle, &resbuf, 1, on_write);
      uv_fs_req_cleanup(fs_req);
      free(fs_req);
      return;
    }

    char path[MAX_PATH_LENGTH];
    req->length_path =
        snprintf(path, MAX_PATH_LENGTH, "%s%s%s", web_config->www_root,
                 req->url, web_config->defaults[req->try_default]);

    if (req->length_path >= MAX_PATH_LENGTH) {
      // FIXME: response 500
      client_t *client = req->client;
      uv_buf_t resbuf[5];
      resbuf[0] = uv_buf_init("HTTP/1.1 500 Internal Server Error\r\n", 36);
      resbuf[1] = uv_buf_init("Content-Type: text/html\r\n", 25);
      resbuf[2] = uv_buf_init("Content-Length: 197\r\n", 21);
      resbuf[3] = uv_buf_init("\r\n", 2);
      resbuf[4] = uv_buf_init(
          "<html><header><title>MingleJet</title></header>"
          "<body>"
          "<h1>500 Internal Server Error</h1>"
          "<p>An unexpected error occurred while processing your request.</p>"
          "<p>Please try again later.</p>"
          "</body>"
          "</html>",
          197);
      // send response
      uv_write_t *write_req = malloc(sizeof(uv_write_t));
      write_req->data = (void *)client;
      uv_write(write_req, (uv_stream_t *)&client->handle, &resbuf[0], 5,
               on_write);
      uv_fs_req_cleanup(fs_req);
      free(fs_req);
      return;
    }

    // next default req
    fprintf(stdout, "Find default file:%s\n", fs_req->path);
    uv_fs_t *new_req = malloc(sizeof(uv_fs_t));
    new_req->data = req;
    uv_fs_stat(loop, new_req, path, check_default_files_async);

    uv_fs_req_cleanup(fs_req);
    free(fs_req);
    return;
  }

  // find file will send the file to user
  req->length_content = fs_req->statbuf.st_size;
  req->path_content = strdup(fs_req->path);
  found_and_sendfs_req(req);

  uv_fs_req_cleanup(fs_req);
  free(fs_req);
}

static void check_path_async(uv_fs_t *fs_req) {
  request_t *req = (request_t *)fs_req->data;
  client_t *client = req->client;

  if (fs_req->result < 0) {
    fprintf(stdout, "check fs_stat failed\n");

    uv_buf_t resbuf = uv_buf_init((char *)response404, strlen(response404));

    // send reponse;
    uv_write_t *write_req = malloc(sizeof(uv_write_t));
    write_req->data = (void *)client;
    uv_write(write_req, (uv_stream_t *)&client->handle, &resbuf, 1, on_write);
    uv_fs_req_cleanup(fs_req);
    free(fs_req);
    return;
  }

  const uv_stat_t *stat = &fs_req->statbuf;
  if (S_ISDIR(stat->st_mode)) {
    req->try_default = 0;

    char path[MAX_PATH_LENGTH];
    req->length_path = snprintf(path, MAX_PATH_LENGTH, "%s%s", fs_req->path,
                                web_config->defaults[req->try_default]);
    fprintf(stdout, "try to find default file%s\n", path);

    uv_fs_t *new_req = malloc(sizeof(uv_fs_t));
    new_req->data = req;
    uv_fs_stat(loop, new_req, path, check_default_files_async);

    uv_fs_req_cleanup(fs_req);
    free(fs_req);
    return;
  } else {
    // found a file
    req->length_content = fs_req->statbuf.st_size;
    req->path_content = strdup(fs_req->path);
    found_and_sendfs_req(req);
  }

  uv_fs_req_cleanup(fs_req);
  free(fs_req);
}

static void process_request(llhttp_t *parser, request_t *req) {
  fprintf(stdout, "Parse pass, type:%d, method:%d, url: %s\n", parser->type,
          parser->method, req->url);

  char path[MAX_PATH_LENGTH];

  req->length_path =
      snprintf(path, MAX_PATH_LENGTH, "%s%s", web_config->www_root, req->url);
  if (req->length_path >= MAX_PATH_LENGTH) {
    client_t *client = req->client;
    uv_buf_t resbuf[5];
    resbuf[0] = uv_buf_init("HTTP/1.1 500 Internal Server Error\r\n", 36);
    resbuf[1] = uv_buf_init("Content-Type: text/html\r\n", 25);
    resbuf[2] = uv_buf_init("Content-Length: 130\r\n", 21);
    resbuf[3] = uv_buf_init("\r\n", 2);
    resbuf[4] = uv_buf_init(
        "<h1>500 Internal Server Error</h1>"
        "<p>An unexpected error occurred while processing your request.</p>"
        "<p>Please try again later.</p>",
        130);
    // send response
    uv_write_t *write_req = malloc(sizeof(uv_write_t));
    write_req->data = (void *)client;
    uv_write(write_req, (uv_stream_t *)&client->handle, &resbuf[0], 5,
             on_write);
    return;
  }

  uv_fs_t *fs_req = malloc(sizeof(uv_fs_t));
  fs_req->data = req;
  uv_fs_stat(loop, fs_req, path, check_path_async);
}

// Callback to handle HTTP method
int on_message_begin(llhttp_t *parser) {
  printf("on_message_begin HTTP method: %ul\n", parser->method);
  return 0;
}

// Main callback to handle request complete
static int on_message_complete(llhttp_t *parser) {
  UNUSED(parser);
  printf("Request complete\n");
  return 0;
}

// Callback to handle URL
int on_url(llhttp_t *parser, const char *at, size_t length) {
  client_t *client = (client_t *)parser->data;
  if (client->request.url != (char *)NULL)
    free(client->request.url);
  client->request.length_url = (uint32_t)length;
  client->request.url = strndup(at, length);
  return 0;
}

// Callback to handle HTTP version
int on_status(llhttp_t *parser, const char *at, size_t length) {
  UNUSED(parser);
  printf("HTTP version: %.*s\n", (int)length, at);
  return 0;
}

// Callback to handle header field
int on_header_field(llhttp_t *parser, const char *at, size_t length) {
  UNUSED(parser);
  // printf("Header field: %.*s\n", (int)length, at);
  return 0;
}

// Callback to handle header value
int on_header_value(llhttp_t *parser, const char *at, size_t length) {
  UNUSED(parser);
  // printf("Header value: %.*s\n", (int)length, at);
  return 0;
}

static int on_headers_complete(llhttp_t *parser) {
  UNUSED(parser);
  printf("Headers complete\n");
  return 0;
}

static int on_body(llhttp_t *parser, const char *at, size_t length) {
  UNUSED(parser);
  UNUSED(at);
  UNUSED(length);
  return 0;
}

/* -------------------------------------------------------------------------------------------
 */
// static void on_stat(uv_fs_t *req) {
//   client_t *client = req->data;
//   client->filesize = 0;
//   if (req->result < 0) {
//     // Error
//     fprintf(stderr, "stat error: %s\n", uv_strerror(req->result));
//   } else {
//     // Send file
//     client->filesize = uv_fs_get_statbuf(req)->st_size;
//   }
//   uv_fs_req_cleanup(req);
// }

// static void on_open(uv_fs_t *req) {
//   // client_t *client = req->data;
//   if (req->result < 0) {
//     // Error
//     fprintf(stderr, "stat error: %s\n", uv_strerror(req->result));
//   } else {
//     ;
//   }
//   uv_fs_req_cleanup(req);
// }

int open_file(client_t *client, const char *file_path) {
  // non callback function will blocking
  const int ret = uv_fs_open(loop, &client->req_open, file_path, O_RDONLY,
                             (S_IRUSR | S_IRGRP), NULL);
  uv_fs_req_cleanup(&client->req_open);
  return ret;
}

int stat_file(client_t *client, const char *file_path) {
  // non callback function will blocking
  const int ret = uv_fs_stat(loop, &client->req_stat, file_path, NULL);
  client->filesize = 0;
  if (ret < 0)
    return 0;

  client->filesize = uv_fs_get_statbuf(&client->req_stat)->st_size;
  uv_fs_req_cleanup(&client->req_stat);
  return ret;
}

int close_file(client_t *client) {
  return uv_fs_close(loop, &client->req_close, client->req_open.result, NULL);
}

static void on_alloc(uv_handle_t *handle, size_t suggested_size,
                     uv_buf_t *buf) {
  UNUSED(handle);
  *buf = uv_buf_init((char *)malloc(suggested_size), suggested_size);
}

static void on_close(uv_handle_t *handle) {
  client_t *client = (client_t *)(handle->data);
  // move client from activeCLientList to freeClientList
  LL_DELETE(activeClientList, client);
  LL_APPEND(freeClientList, client);
}

static void on_sendfile(uv_fs_t *req) {
  client_t *client = (client_t *)req->data;

  uv_fs_t req_close;
  uv_fs_close(loop, &req_close, client->req_open.result, NULL);
  uv_fs_req_cleanup(req);
  free(req);
  client->refCounter--;
}

static void on_write(uv_write_t *req, int status) {
  client_t *client = (client_t *)req->data;

  if (status == 0 && client->sendfile) {
    uv_os_fd_t sendfd;
    client->req_sendfile = (uv_fs_t *)malloc(sizeof(uv_fs_t));
    client->req_sendfile->data = (void *)client;
    uv_fileno((uv_handle_t *)&client->handle, &sendfd);
    client->sendfile = false;
    client->refCounter++;
    uv_fs_sendfile(loop, client->req_sendfile, sendfd, client->req_open.result,
                   0, client->filesize, on_sendfile);
  }
  free(req);
}

// Callback to handle HTTP request data
void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
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

  llhttp_t *parser = &client->parser;
  // Parse the received data
  enum llhttp_errno err = llhttp_execute(parser, buf->base, nread);
  if (err != HPE_OK) {
    fprintf(stderr, "Parse error: %s %s\n", llhttp_errno_name(err),
            client->parser.reason);
    uv_close((uv_handle_t *)&client->handle, (uv_close_cb)on_close);
    free(buf->base);
    return;
  }

  client->request.client = client;
  // parsed successfully
  process_request(parser, &client->request);
  free(buf->base);
}

static client_t *createClient(void) {
  client_t *client = (client_t *)calloc(1, sizeof(client_t));
  if (client != NULL) {
    LL_APPEND(activeClientList, client);
    return client;
  }
  return NULL;
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

  client_t *client = createClient();
  uv_tcp_init(loop, &client->handle);
  llhttp_init(&client->parser, HTTP_REQUEST, &settings);
  client->handle.data = client;
  client->parser.data = client;
  if (uv_accept(server, (uv_stream_t *)client) == 0) {
    uv_read_start((uv_stream_t *)&(client->handle), on_alloc, on_read);
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

void signal_handler(uv_signal_t *handle, int signum) {
  UNUSED(signum);
  uv_stop(loop);
}

/**
 * @brief Starts the web server.
 *
 * This function initializes the libuv event loop, sets up HTTP parser settings,
 * initializes a TCP server, binds it to the specified address and port,
 * listens for incoming connections, and starts the event loop to handle client
 * requests.
 *
 * @param config Pointer to the web server configuration structure.
 *               If NULL, default configuration will be used.
 *
 * @return Returns 0 upon successful execution, or a non-zero value if an error
 *         occurs.
 */
int webserver(webconfig_t *config) {
  // Initialize libuv event loop
  loop = uv_default_loop();

  // Set configuration if provided
  if (config != NULL) {
    web_config = config;
  } else {
    // Use default configuration
    // (optional: set default configuration)
  }

  // Initialize signal handlers
  uv_signal_init(loop, &sigint_handle);
  uv_signal_init(loop, &sigterm_handle);

  // Register signal handlers
  uv_signal_start(&sigint_handle, signal_handler, SIGINT);
  uv_signal_start(&sigterm_handle, signal_handler, SIGTERM);

  // Initialize HTTP parser settings
  llhttp_settings_init(&settings);
  settings.on_message_begin = on_message_begin;
  settings.on_url = on_url;
  settings.on_status = on_status;
  settings.on_header_field = on_header_field;
  settings.on_header_value = on_header_value;
  settings.on_message_complete = on_message_complete;
  settings.on_headers_complete = on_headers_complete;
  settings.on_body = on_body;

  // Initialize TCP server
  uv_tcp_t server;
  uv_tcp_init(loop, &server);

  // Bind server to specified address and port
  struct sockaddr_in bind_addr;
  uv_ip4_addr("0.0.0.0", DEFAULT_PORT, &bind_addr);
  uv_tcp_bind(&server, (const struct sockaddr *)&bind_addr, 0);

  // Start listening for incoming connections
  int r = uv_listen((uv_stream_t *)&server, SOMAXCONN, on_connection);
  if (r) {
    fprintf(stderr, "Listen error %s\n", uv_strerror(r));
    return -1;
  }

  // Print server information
  fprintf(stdout, "Launch MingleJet...\n\n");
  fprintf(stdout, "use the below third party components\n");
  fprintf(stdout, "  libuv:%d.%d.%d %s\n", UV_VERSION_MAJOR, UV_VERSION_MINOR,
          UV_VERSION_PATCH, UV_VERSION_IS_RELEASE ? "Release" : "Testing");
  fprintf(stdout, "  llhttp:%d.%d.%d\n", LLHTTP_VERSION_MAJOR,
          LLHTTP_VERSION_MINOR, LLHTTP_VERSION_PATCH);

// Print third-party component versions
#define STR_HELPER(x) #x
#define STR_VERSION(x) STR_HELPER(x)
  fprintf(stdout, "  utlist:%s\n\n", STR_VERSION(UTLIST_VERSION));
#undef STR_HELPER
#undef STR_VERSION

  // Print server listening information
  fprintf(stdout, "Server listening on port %d...\n", DEFAULT_PORT);

  // Setup idle handle for cleanup
  setup_cleanup_idle(loop);

  // Run libuv event loop
  int ret = uv_run(loop, UV_RUN_DEFAULT);

  // Stop idle handle
  uv_idle_stop(idle_handle);

  // Release resources
  uv_walk(loop, walk_cb, 0);
  uv_run(loop, UV_RUN_DEFAULT); // Run pending callbacks
  cleanup_resources();
  free(idle_handle);

  // Clean up resources and close event loop
  // the following will release in uv_walk()
  // uv_signal_stop(&sigint_handle);
  // uv_signal_stop(&sigterm_handle);
  uv_loop_close(loop);
  fprintf(stdout, "Server Shutdown now\n");
  return ret;
}
