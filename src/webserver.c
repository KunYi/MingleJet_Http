
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* include libuv & llhttp */
#include "defineds.h"
#include "utils.h"
#include "webserver.h"

static void get_param_cleanup(get_param_t *param) {
  free(param->name);
  free(param->value);
}

static UT_icd get_params_icd = {sizeof(get_param_t), NULL, NULL,
                                (void (*)(void *))get_param_cleanup};

static const char *res404content = "<!DOCTYPE html>"
                                   "<html>"
                                   "<header>"
                                   "<title>MingleJet</title>"
                                   "</header>"
                                   "<body>"
                                   "<H1>Not Found</H1>"
                                   "</body>"
                                   "</html>";

static const char *res500content =
    "<!DOCTYPE html>"
    "<html>"
    "<header>"
    "<title>MingleJet</title>"
    "</header>"
    "<body>"
    "<h1>500 Internal Server Error</h1>"
    "<p>An unexpected error occurred while processing your request.</p>"
    "<p>Please try again later.</p>"
    "</body>"
    "</html>";

static const char *response401 = "HTTP/1.1 401 Unauthorized\r\n"
                                 "Location: %s\r\n"
                                 "Content-Length: 0\r\n"
                                 "\r\n";

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
    {".ico", "image/x-icon"},
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

/* http status codes */
static const http_status_code_t status1xx_codes[] = {
    // Informational responses
    {100, "Continue"},
    {101, "Switching Protocols"},
    {102, "Processing"},
    {103, "Early Hints"},
};
static const int num_status1xx_codes =
    sizeof(status1xx_codes) / sizeof(http_status_code_t);

static const http_status_code_t status2xx_codes[] = {
    // Successful responses
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status (WebDAV)"},
    {208, "Already Reported (WebDAV)"},
    {226, "IM Used"},
};
static const int num_status2xx_codes =
    sizeof(status2xx_codes) / sizeof(http_status_code_t);

static const http_status_code_t status3xx_codes[] = {
    // Redirection messages
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {306, "(Unused)"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},
};
static const int num_status3xx_codes =
    sizeof(status3xx_codes) / sizeof(http_status_code_t);

static const http_status_code_t status4xx_codes[] = {
    // Client error responses
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "I'm a teapot"},
    {419, "Authentication Timeout"},
    {421, "Misdirected Request"},
    {422, "Unprocessable Entity"},
    {423, "Locked"},
    {424, "Failed Dependency"},
    {425, "Unordered Collection"},
    {426, "Upgrade Required"},
    {428, "Precondition Required"},
    {429, "Too Many Requests"},
    {431, "Request Header Fields Too Large"},
    {440, "Login Timeout"},
    {444, "No Response"},
    {450, "Blocked by Windows Parental Controls"},
    {451, "Unavailable For Legal Reasons"},
    {452, "Request Header Fields Too Large"},
    {494, "Request Header Timeout"},
    {495, "Cert Error"},
    {496, "Client Closed Request"},
    {497, "HTTP Request Sent To HTTPS Port"},
    {499, "Client Closed Request"},
};
static const int num_status4xx_codes =
    sizeof(status4xx_codes) / sizeof(http_status_code_t);

static const http_status_code_t status5xx_codes[] = {
    // Server error responses
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storag"},
    {508, "Loop Detected"},
    {510, "Not Extended"},
    {511, "Network Authentication Required"}};
static const int num_status5xx_codes =
    sizeof(status5xx_codes) / sizeof(http_status_code_t);

static webconfig_t *web_config;
static uv_loop_t *loop;
static uv_signal_t sigint_handle, sigterm_handle;
static uv_timer_t release_timer;
// HTTP parser settings
static llhttp_settings_t settings;

static void on_write(uv_write_t *req, int status);

static client_t *activeClientList = NULL;

static void free_client(client_t *client) {
  if (client->request.url != NULL) {
    free(client->request.url);
    client->request.url = NULL;
  }
  if (client->request.body != NULL) {
    free(client->request.body);
    client->request.body = NULL;
  }
  if (client->request.query_param != NULL) {
    utarray_free(client->request.query_param);
  }
  free(client);
}

static void cleanup_freeList(uv_timer_t *handle) {
  UNUSED(handle);
  // clean
  client_t *elt, *tmp;
  LL_FOREACH_SAFE(activeClientList, elt, tmp) {
    if (CLIENT_IS_FLAGS_FREE(elt)) {
      LL_DELETE(activeClientList, elt);
      free_client(elt);
    }
  }
}

static void cleanup_resources(void) {
  client_t *elt, *tmp;
  LL_FOREACH_SAFE(activeClientList, elt, tmp) {
    LL_DELETE(activeClientList, elt);
    free_client(elt);
  }
}

static void setup_cleanup_timer(uv_loop_t *loop) {
  // Release resource1 after 200ms
  uv_timer_init(loop, &release_timer);
  uv_timer_start(&release_timer, (uv_timer_cb)&cleanup_freeList, 200, 200);
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

static const char *status_string(llhttp_status_t status) {
  const http_status_code_t *p = NULL;
  int max_statuscode = 0;

  if ((status >= 100) && (status < 200)) {
    p = &status1xx_codes[0];
    max_statuscode = num_status1xx_codes;
  } else if ((status >= 200) && (status < 300)) {
    p = &status2xx_codes[0];
    max_statuscode = num_status2xx_codes;
  } else if ((status >= 300) && (status < 400)) {
    p = &status3xx_codes[0];
    max_statuscode = num_status3xx_codes;
  } else if ((status >= 400) && (status < 500)) {
    p = &status4xx_codes[0];
    max_statuscode = num_status4xx_codes;
  } else if ((status >= 500) && (status < 600)) {
    p = &status5xx_codes[0];
    max_statuscode = num_status5xx_codes;
  } else {
    return "Unknow Status";
  }

  for (int i = 0; i < max_statuscode; i++) {
    if (status == p[i].code)
      return p[i].reason_phrase;
  }
  return "Unknow Status";
}

static const int make_header_status(llhttp_status_t status, char *buf,
                                    uint32_t len) {
  return snprintf(buf, len, "HTTP/1.1 %d %s\r\n", status,
                  status_string(status));
}

static const int make_header_content_type(const char *content_type, char *buf,
                                          uint32_t len) {
  return snprintf(buf, len, "Content-Type: %s\r\n", content_type);
}

static const int make_header_content_length(size_t content_length, char *buf,
                                            uint32_t len) {
  return snprintf(buf, len, "Content-Length: %ld\r\n", content_length);
}

static uv_buf_t make_response_header(llhttp_status_t status, response_t *res) {
  if (res == NULL) {
    return uv_buf_init(NULL, 0);
  }

  char buf[2048];
  char *ret = buf;
  int len = sizeof(buf);
  int cnt = 0;

  if (ret != NULL) {
    cnt = make_header_status(status, ret, len);
    len -= cnt;
    if (res->mime_content != NULL) {
      cnt += make_header_content_type(res->mime_content, ret + cnt, len);
      len -= cnt;
    }
    // always include 'Content-Length' field, even the value is zero
    cnt += make_header_content_length(res->size_content, ret + cnt, len);
    len -= cnt;
    cnt += snprintf(ret + cnt, len, "\r\n");
  }

  uv_buf_t uv_buf = uv_buf_init(malloc(cnt), cnt);
  strncpy(uv_buf.base, buf, cnt);
  return uv_buf;
}

static void on_final_fix_response(uv_write_t *req, int status) {
  if (status == 0) {
    assert(req->nbufs == 2);
    client_t *client = (client_t *)req->data;
    free(client->response.buf[0].base);

    // the content for pre-defined fixed address
    // not in heap/malloc
    // free(client->response.buf[1].base);
    free(client->response.buf);
    client->response.buf = NULL;
  }
  free(req);
}

static void make_fixed_response(client_t *client, const llhttp_status_t code,
                                const char *mime_type, const char *content) {
  response_t *res = &client->response;
  const size_t len = strlen(content);

  res->buf = malloc(2 * sizeof(uv_buf_t));
  res->mime_content = mime_type;
  res->size_content = len;
  res->buf[0] = make_response_header(code, res);
  res->buf[1] = uv_buf_init((char *)content, len);

  // send response
  uv_write_t *write_req = malloc(sizeof(uv_write_t));
  write_req->data = (void *)client;
  uv_write(write_req, (uv_stream_t *)&client->handle, res->buf, 2,
           on_final_fix_response);
}

static void send_text_response(client_t *client, const llhttp_status_t code,
                               const char *content) {
  make_fixed_response(client, code, match_mime_type(".txt"), content);
}

static void send_html_response(client_t *client, const llhttp_status_t code,
                               const char *content) {
  make_fixed_response(client, code, match_mime_type(".html"), content);
}

static void on_close_sendfile(uv_fs_t *fs_req) {
  uv_fs_req_cleanup(fs_req);
  free(fs_req);
}

static void final_sendfile(uv_fs_t *fs_req) {
  client_t *client = (client_t *)fs_req->data;
  const response_t *res = &client->response;

  uv_fs_t *req_close = (uv_fs_t *)malloc(sizeof(uv_fs_t));
  uv_fs_close(loop, req_close, res->open_file, on_close_sendfile);
  uv_fs_req_cleanup(fs_req);
  free(fs_req);
  CLIENT_CLEAR_IN_REF(client);
}

static void send_file_context(uv_fs_t *fs_req) {
  client_t *client = (client_t *)fs_req->data;
  response_t *res = &client->response;

  // FIXME: only for linux
  if (fs_req->result >= 0) {
    uv_os_fd_t sendfd;
    uv_fs_t *send_req = (uv_fs_t *)malloc(sizeof(uv_fs_t));
    send_req->data = client;
    uv_fileno((uv_handle_t *)&client->handle, &sendfd);
    CLIENT_SET_IN_REF(client);
    res->open_file = fs_req->result; // store the file handler
    uv_fs_sendfile(loop, send_req, sendfd, fs_req->result, 0, res->size_content,
                   final_sendfile);
#ifdef _WIN32
#error "because windows not support sendfile(), need implement"
#endif
  }

  // release path
  free(res->path_content);
  res->path_content = NULL;

  uv_fs_req_cleanup(fs_req);
  free(fs_req);
}

static void open_send_file(uv_write_t *req, int status) {
  client_t *client = (client_t *)req->data;
  response_t *res = &client->response;
  if (status == 0) {
    uv_fs_t *fs_req = malloc(sizeof(uv_fs_t));
    fs_req->data = client;
    uv_fs_open(loop, fs_req, res->path_content, O_RDONLY, (S_IRUSR | S_IRGRP),
               send_file_context);
  }

  free(res->buf->base);
  free(res->buf);
  free(req);
}

static void found_and_sendfs_req(client_t *client) {
  response_t *res = &client->response;
  res->buf = malloc(sizeof(uv_buf_t));
  *res->buf = make_response_header(HTTP_STATUS_OK, res);
  // send header of response;
  uv_write_t *write_req = malloc(sizeof(uv_write_t));
  write_req->data = (void *)client;
  uv_write(write_req, (uv_stream_t *)&client->handle, res->buf, 1,
           open_send_file);
}

static void check_default_files_async(uv_fs_t *fs_req) {
  client_t *client = (client_t *)fs_req->data;
  request_t *req = &client->request;
  response_t *res = &client->response;

  if (fs_req->result != 0) {
    // fprintf(stdout, "Can't find file: %s\n", fs_req->path);
    req->default_filename_tries++;
    if (req->default_filename_tries >= web_config->def_cnt) {
      send_html_response(client, HTTP_STATUS_NOT_FOUND, res404content);
      uv_fs_req_cleanup(fs_req);
      free(fs_req);
      return;
    }

    char path[MAX_PATH_LENGTH];
    const int len = strlen(req->url);
    if (len > 1 && req->url[len - 1] != '/') {
      res->length_path =
          snprintf(path, MAX_PATH_LENGTH, "%s%s/%s", web_config->www_root,
                   req->url, web_config->defaults[req->default_filename_tries]);
    } else {
      res->length_path =
          snprintf(path, MAX_PATH_LENGTH, "%s%s%s", web_config->www_root,
                   req->url, web_config->defaults[req->default_filename_tries]);
    }

    if (res->length_path >= MAX_PATH_LENGTH) {
      send_html_response(client, HTTP_STATUS_INTERNAL_SERVER_ERROR,
                         res500content);
      uv_fs_req_cleanup(fs_req);
      free(fs_req);
      return;
    }

    // next default req
    fprintf(stdout, "try next default file:%s\n", path);
    uv_fs_t *new_req = malloc(sizeof(uv_fs_t));
    new_req->data = client;
    uv_fs_stat(loop, new_req, path, check_default_files_async);

    uv_fs_req_cleanup(fs_req);
    free(fs_req);
    return;
  }

  // find file will send the file to user
  res->size_content = fs_req->statbuf.st_size;
  res->path_content = strdup(fs_req->path);
  res->mime_content = match_mime_type(fs_req->path);
  found_and_sendfs_req(client);

  uv_fs_req_cleanup(fs_req);
  free(fs_req);
}

static void check_path_async(uv_fs_t *fs_req) {
  client_t *client = (client_t *)fs_req->data;
  request_t *req = &client->request;
  response_t *res = &client->response;

  if (fs_req->result < 0) {
    fprintf(stdout, "check fs_stat failed\n");
    send_html_response(client, HTTP_STATUS_NOT_FOUND, res404content);
    uv_fs_req_cleanup(fs_req);
    free(fs_req);
    return;
  }

  const uv_stat_t *stat = &fs_req->statbuf;
  if (S_ISDIR(stat->st_mode)) {
    req->default_filename_tries = 0;

    char path[MAX_PATH_LENGTH];
    const int len = strlen(fs_req->path);
    if (len > 1 && fs_req->path[len - 1] != '/') {
      res->length_path =
          snprintf(path, MAX_PATH_LENGTH, "%s/%s", fs_req->path,
                   web_config->defaults[req->default_filename_tries]);
    } else {
      res->length_path =
          snprintf(path, MAX_PATH_LENGTH, "%s%s", fs_req->path,
                   web_config->defaults[req->default_filename_tries]);
    }
    fprintf(stdout, "try to find default file%s\n", path);

    uv_fs_t *new_req = malloc(sizeof(uv_fs_t));
    new_req->data = client;
    uv_fs_stat(loop, new_req, path, check_default_files_async);

    uv_fs_req_cleanup(fs_req);
    free(fs_req);
    return;
  } else {
    // found a file
    res->size_content = fs_req->statbuf.st_size;
    res->path_content = strdup(fs_req->path);
    res->mime_content = match_mime_type(fs_req->path);
    found_and_sendfs_req(client);
  }

  uv_fs_req_cleanup(fs_req);
  free(fs_req);
}

int handle_healthcheck(client_t *client) {
  request_t *req = &client->request;

  if ((req->method == HTTP_GET) &&
      (strncmp(req->url, "/healthcheck", 12) == 0)) {
    send_text_response(client, HTTP_STATUS_OK, "OK");
    return 0;
  }
  return -1;
}

int handle_create_todo(client_t *client) {
  request_t *req = &client->request;

  if ((req->method == HTTP_POST) &&
      (strncmp(req->url, "/api/todos", 10) == 0) && (req->length_url == 10)) {
    json_error_t error;
    json_t *obj = json_loads(req->body, 0, &error);
    if (obj != NULL) {
      todo_t *todo = create_todo_from_json(obj);
      json_decref(obj);
      if (todos_append(todo) == 0) {
        char *jsonstr = todos_dump();
        if (jsonstr) {
          send_text_response(client, HTTP_STATUS_CREATED, jsonstr);
          free(jsonstr);
          return 0;
        }
      }
    } else {
      fprintf(stdout, "parsing failed body\n%s\n", req->body);
    }
    fprintf(stdout, "Failed to call api '/api/todos'\nbody:%s\n", req->body);
    // Handle potential errors (e.g., invalid JSON, already exists)
    send_text_response(client, HTTP_STATUS_BAD_REQUEST, "Invalid id");
    return 0;
  }
  return -1;
}

int handle_get_todos(client_t *client) {
  request_t *req = &client->request;
  if ((req->method == HTTP_GET) && (strncmp(req->url, "/api/todos", 10) == 0) &&
      (req->length_url == 10)) {
    char *jsonstr = todos_dump();
    if (jsonstr) {
      send_text_response(client, HTTP_STATUS_ACCEPTED, jsonstr);
      free(jsonstr);
      return 0;
    }
    send_text_response(client, HTTP_STATUS_INTERNAL_SERVER_ERROR,
                       "failed to read");
    return 0;
  }

  return -1;
}

static int validate_todo_patch(const char *url) {
  if (strncmp(url, "/api/todos/", 11) == 0) {
    char *done = strrchr(url, '/');
    if (done && strncmp(done, "/done", 5) == 0) {
      return 1; // api
    }
    return 0;
  }
  return 0;
}

int handle_patch_todo(client_t *client) {
  request_t *req = &client->request;
  if ((req->method == HTTP_PATCH) && validate_todo_patch(req->url)) {
    // convert id
    unsigned int id = 0;
    id = strtoul(req->url + 11, NULL, 10);
    if (id == 0) {
      send_text_response(client, HTTP_STATUS_BAD_REQUEST, "Invalid id");
      return 0;
    }
    todo_update(id, NULL, 1, NULL);
    char *jsonstr = todos_dump();
    if (jsonstr) {
      send_text_response(client, HTTP_STATUS_ACCEPTED, jsonstr);
      free(jsonstr);
      return 0;
    }
    send_text_response(client, HTTP_STATUS_INTERNAL_SERVER_ERROR,
                       "failed to read");
    return 0;
  }

  return -1;
}

static void process_request(llhttp_t *parser, client_t *client) {
  request_t *req = &client->request;
  response_t *res = &client->response;
  fprintf(stdout, "Parse pass, type:%d, method:%d, url: %s\n", parser->type,
          parser->method, req->url);

   if (handle_healthcheck(client) == 0)
     return;
   else if (handle_create_todo(client) == 0)
     return;
   else if (handle_get_todos(client) == 0)
     return;
   else if (handle_patch_todo(client) == 0)
     return;

  char path[MAX_PATH_LENGTH];

  res->length_path =
      snprintf(path, MAX_PATH_LENGTH, "%s%s", web_config->www_root, req->url);
  if (res->length_path >= MAX_PATH_LENGTH) {
    send_html_response(client, HTTP_STATUS_INTERNAL_SERVER_ERROR,
                       res500content);
    return;
  }

  uv_fs_t *fs_req = malloc(sizeof(uv_fs_t));
  fs_req->data = client;
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

static void parse_get_url(const char *url, UT_array *array) {
  char *token;
  // Make a copy to avoid modifying the original string
  char *url_copy = strdup(url);
  char *saveptr;

  token = strtok_r(url_copy, "&", &saveptr); // Split the string by '&'

  while (token != NULL) {
    char *param_name =
        strtok(token, "="); // Split each token by '=' to get parameter name
    char *param_value = strtok(NULL, "="); // Get parameter value

    if (param_name != NULL && param_value != NULL) {
      get_param_t param;
      param.name = strdup(param_name);
      param.value = strdup(param_value);
      utarray_push_back(array, &param);
    }

    token = strtok_r(NULL, "&", &saveptr);
  }

  free(url_copy); // Free the memory allocated for the copied string
}

// Callback to handle URL
int on_url(llhttp_t *parser, const char *at, size_t length) {
  client_t *client = (client_t *)parser->data;
  if (client->request.url != (char *)NULL)
    free(client->request.url);

  char *path = strndup(at, length);
  const char *question = strchr(path, '?');
  char *url = NULL;
  if (question != NULL) {
    const int len = question - path;
    char *t_url = (char *)malloc(len + 1);
    strncpy(t_url, path, len);
    t_url[len] = '\0';
    url = validate_and_normalize_path(t_url);
    free(t_url);
    utarray_new(client->request.query_param, &get_params_icd);
    parse_get_url(question + 1, client->request.query_param);
    free(path);
  } else {
    url = validate_and_normalize_path(path);
    free(path);
  }

  if (url != NULL) {
    path = strdup(url);
    free(url);
    client->request.length_url = (uint32_t)strlen(path);
    client->request.url = path;
  } else {
    client->request.url = strdup("/");
    client->request.length_url = strlen(client->request.url);
  }
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
  client_t *client = (client_t *)parser->data;
  if (at != NULL && length > 0) {
    client->request.body = strndup(at, length);
    client->request.length_body = length;
  }
  return 0;
}

static void on_alloc(uv_handle_t *handle, size_t suggested_size,
                     uv_buf_t *buf) {
  UNUSED(handle);
  *buf = uv_buf_init((char *)malloc(suggested_size), suggested_size);
}

static void on_close(uv_handle_t *handle) {
  client_t *client = (client_t *)(handle->data);
  CLIENT_CLEAR_IN_USE(client);
  if (CLIENT_IS_FLAGS_FREE(client)) {
    LL_DELETE(activeClientList, client);
    free_client(client);
  }
}

static void on_write(uv_write_t *req, int status) {
  if (status == 0) {
    uv_buf_t *ptr = (uv_buf_t *)req->data;
    for (unsigned int i = 0; i < req->nbufs; ++i) {
      free(ptr[i].base);
    }
    free(ptr);
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

  client->request.method = parser->method;
  // parsed successfully
  process_request(parser, client);
  free(buf->base);
}

static client_t *createClient(void) {
  client_t *client = (client_t *)calloc(1, sizeof(client_t));
  if (client != NULL) {
    CLIENT_SET_IN_USE(client);
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
  setup_cleanup_timer(loop);

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
