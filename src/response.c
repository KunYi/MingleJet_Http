#include "response.h"

#define STATUS_STRING_AVAILABLE (1)

typedef struct mime_type_pair_s {
  const char *ext;
  const char *type;
} mime_type_pair_t;

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

const char *match_mime_type(const char *path) {
  if (!path) return "application/octet-stream";
  const char *ext = strrchr(path, '.');
  if (!ext) return "application/octet-stream";

  for (size_t i = 0; i < sizeof(mime_types) / sizeof(mime_types[0]); i++) {
    if (strcasecmp(ext, mime_types[i].ext) == 0) {
      return mime_types[i].type;
    }
  }
  return "application/octet-stream";
}

static const char *status_string(llhttp_status_t status) {
    switch (status) {
#define XX(num, name, str) \
    case HTTP_STATUS_##name: \
        return #str;
        HTTP_STATUS_MAP(XX)
#undef XX
    }
    return "unknown";
}


/* header helpers */
static int make_header_status(int status, char *buf, uint32_t len) {
  /* assume status_string() exists in project; otherwise use numeric reason */
#ifdef STATUS_STRING_AVAILABLE
  return snprintf(buf, len, "HTTP/1.1 %d %s\r\n", status, status_string(status));
#else
  return snprintf(buf, len, "HTTP/1.1 %d\r\n", status);
#endif
}

static int make_header_content_type(const char *content_type, char *buf, uint32_t len) {
  return snprintf(buf, len, "Content-Type: %s\r\n", content_type);
}

static int make_header_content_length(size_t content_length, char *buf, uint32_t len) {
  return snprintf(buf, len, "Content-Length: %zu\r\n", content_length);
}

/* Build response header into a uv_buf_t (malloc'ed) */
uv_buf_t make_response_header(int status, response_t *res) {
  char tmp[2048];
  int used = 0;
  int n;

  n = make_header_status(status, tmp + used, sizeof(tmp) - used);
  if (n > 0) { used += n; }

  if (res && res->mime_content) {
    n = make_header_content_type(res->mime_content, tmp + used, sizeof(tmp) - used);
    if (n > 0) { used += n; }
  }

  if (res) {
    n = make_header_content_length(res->size_content, tmp + used, sizeof(tmp) - used);
    if (n > 0) { used += n; }
  }

  /* finish header */
  n = snprintf(tmp + used, sizeof(tmp) - used, "\r\n");
  if (n > 0) used += n;

  uv_buf_t b = uv_buf_init(malloc(used), used);
  memcpy(b.base, tmp, used);
  return b;
}

/* send a small fixed HTML response (404/500) synchronously via uv_write (async, but small) */
static void on_final_fix_response(uv_write_t *req, int status) {
  if (status == 0) {
    client_t *client = (client_t *)req->data;
    /* free header buffer */
    if (client && client->response.buf) {
      free(client->response.buf[0].base);
      free(client->response.buf);
      client->response.buf = NULL;
    }
  }
  free(req);
}

static void make_fixed_response(client_t *client, int code, const char *mime_type, const char *content) {
  response_t *res = &client->response;
  size_t len = content ? strlen(content) : 0;

  res->buf = malloc(2 * sizeof(uv_buf_t));
  res->mime_content = mime_type;
  res->size_content = len;
  res->buf[0] = make_response_header(code, res);
  res->buf[1] = uv_buf_init((char *)content, (unsigned int)len);

  uv_write_t *w = malloc(sizeof(uv_write_t));
  w->data = (void *)client;
  uv_write(w, (uv_stream_t *)&client->handle, res->buf, 2, on_final_fix_response);
}

void send_text_response(client_t *client, const llhttp_status_t code,
                               const char *content) {
  make_fixed_response(client, code, match_mime_type(".txt"), content);
}

void send_html_response(client_t *client, const llhttp_status_t code,
                               const char *content) {
  make_fixed_response(client, code, match_mime_type(".html"), content);
}
