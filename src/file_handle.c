#include "client_manager.h"
#include "file_handle.h"
#include "default_response.h"
#include "defineds.h"
#include "response.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

// Static variables to hold the loop and config
static uv_loop_t *fs_loop = NULL;
static const webconfig_t *fs_web_config = NULL;

/* Basic path normalization & security:
 * - join root + url
 * - reject path containing ".." components that escape root
 * returns 0 on success and writes result into out (must be MAX_PATH_LENGTH)
 */
static int build_safe_path(const char *root, const char *url, char *out, size_t out_size) {
  if (!root || !url || !out) return -1;
  /* ensure url begins with / */
  const char *u = url;
  if (u[0] != '/') {
    /* prepend slash */
    if (snprintf(out, out_size, "%s/%s", root, url) >= (int)out_size) return -1;
  } else {
    if (snprintf(out, out_size, "%s%s", root, url) >= (int)out_size) return -1;
  }

  /* normalize: simple check - reject any ".." segment */
  if (strstr(out, "/..") != NULL) return -1;
  return 0;
}

/* ---------- async callbacks: full flow ---------- */

/* forward declares (static) */
static void final_sendfile(uv_fs_t *fs_req);
static void send_file_context(uv_fs_t *fs_req);
static void open_send_file(uv_write_t *req, int status);
static void check_default_files_async(uv_fs_t *fs_req);
static void check_path_async(uv_fs_t *fs_req);
static void found_and_sendfs_req(client_t *client);

/* fh_found_and_sendfs_req:
 * build header and write header, callback -> fh_open_send_file will open file and send.
 */
static void found_and_sendfs_req(client_t *client) {
  response_t *res = &client->response;
  /* build response header uv_buf (malloc'd) */
  res->buf = malloc(sizeof(uv_buf_t));
  *res->buf = make_response_header(HTTP_STATUS_OK, res);

  uv_write_t *write_req = malloc(sizeof(uv_write_t));
  write_req->data = (void *)client;
  uv_write(write_req, (uv_stream_t *)&client->handle, res->buf, 1, open_send_file);
}

/* after header write: open file and kick send */
static void open_send_file(uv_write_t *req, int status) {
  client_t *client = (client_t *)req->data;
  response_t *res = &client->response;

  if (status == 0) {
    uv_fs_t *fs_req = malloc(sizeof(uv_fs_t));
    memset(fs_req, 0, sizeof(*fs_req));
    fs_req->data = client;
    /* open read-only, mode ignored for read */
    uv_fs_open(fs_loop, fs_req, res->path_content, O_RDONLY, 0, send_file_context);
  } else {
    /* header write failed */
    send_html_response(client, HTTP_STATUS_INTERNAL_SERVER_ERROR, getResponse500Content());
  }

  /* free header uv_buf (base allocated in fh_make_response_header) */
  if (res->buf) {
    free(res->buf->base);
    free(res->buf);
    res->buf = NULL;
  }
  free(req);
}

/* sendfile completion - close fd and cleanup */
static void final_sendfile(uv_fs_t *fs_req) {
  client_t *client = (client_t *)fs_req->data;
  response_t *res = &client->response;

  uv_fs_t *req_close = (uv_fs_t *)malloc(sizeof(uv_fs_t));
  req_close->data = client;
  uv_fs_close(fs_loop, req_close, res->open_file, /* cb */ NULL);

  uv_fs_req_cleanup(fs_req);
  free(fs_req);

  /* decrease in-ref if project uses client ref counting macros; if present macros will handle */
  client_clear_in_ref(client);
}

/* after uv_fs_open -> sendfile */
static void send_file_context(uv_fs_t *fs_req) {
  client_t *client = (client_t *)fs_req->data;
  response_t *res = &client->response;

  if (fs_req->result >= 0) {
    uv_os_fd_t sendfd;
    uv_fs_t *send_req = malloc(sizeof(uv_fs_t));
    memset(send_req, 0, sizeof(*send_req));
    send_req->data = client;

    /* get socket fd from client handle */
    uv_fileno((const uv_handle_t *)&client->handle, &sendfd);

    /* mark in-ref if project uses such macro */
    client_set_in_ref(client);

    /* store open fd into response so final_close knows which to close */
    res->open_file = (uv_file)fs_req->result;

    /* send entire file (offset 0, length = size_content) */
    uv_fs_sendfile(fs_loop, send_req, sendfd, fs_req->result, 0, res->size_content, final_sendfile);
  } else {
    send_html_response(client, HTTP_STATUS_INTERNAL_SERVER_ERROR, getResponse500Content());
  }

  /* release stored path */
  if (res->path_content) {
    free(res->path_content);
    res->path_content = NULL;
  }

  uv_fs_req_cleanup(fs_req);
  free(fs_req);
}

/* called after uv_fs_stat on default filename candidate */
static void check_default_files_async(uv_fs_t *fs_req) {
  client_t *client = (client_t *)fs_req->data;
  request_t *req = &client->request;
  response_t *res = &client->response;

  /* not found */
  if (fs_req->result != 0) {
    req->default_filename_tries++;
    if (req->default_filename_tries >= (int)fs_web_config->def_cnt) {
      send_html_response(client, HTTP_STATUS_NOT_FOUND, getResponse404Content());
      uv_fs_req_cleanup(fs_req);
      free(fs_req);
      return;
    }

    /* try next default filename */
    char path[MAX_PATH_LENGTH];
    int idx = req->default_filename_tries;
    const char *dfname = fs_web_config->defaults[idx];
    /* path: root + url + "/" + dfname */
    if (req->url[0] == '\0') {
      /* root request */
      snprintf(path, sizeof(path), "%s/%s", fs_web_config->www_root, dfname);
    } else {
      snprintf(path, sizeof(path), "%s%s/%s", fs_web_config->www_root, req->url, dfname);
    }

    uv_fs_req_cleanup(fs_req);
    fs_req->data = client;
    uv_fs_stat(fs_loop, fs_req, path, check_default_files_async);
    return;
  }

  /* found a default file */
  res->size_content = fs_req->statbuf.st_size;
  /* duplicate path (libuv may store pointer internally) */
  res->path_content = strdup(fs_req->path ? fs_req->path : "<unknown>");
  res->mime_content = match_mime_type(fs_req->path ? fs_req->path : "");
  found_and_sendfs_req(client);

  uv_fs_req_cleanup(fs_req);
  free(fs_req);
}

/* called after uv_fs_stat on requested path */
static void check_path_async(uv_fs_t *fs_req) {
  client_t *client = (client_t *)fs_req->data;
  request_t *req = &client->request;
  response_t *res = &client->response;

  if (fs_req->result < 0) {
    send_html_response(client, HTTP_STATUS_NOT_FOUND, getResponse404Content());
    uv_fs_req_cleanup(fs_req);
    free(fs_req);
    return;
  }

  const uv_stat_t *st = &fs_req->statbuf;
  if (S_ISDIR(st->st_mode)) {
    /* directory: try default filenames list */
    req->default_filename_tries = 0;
    char path[MAX_PATH_LENGTH];
    const char *dfname = fs_web_config->defaults[0];
    if (req->url[0] == '\0') {
      snprintf(path, sizeof(path), "%s/%s", fs_web_config->www_root, dfname);
    } else {
      snprintf(path, sizeof(path), "%s%s/%s", fs_web_config->www_root, req->url, dfname);
    }

    /* call stat for default file */
    uv_fs_req_cleanup(fs_req);
    fs_req->data = client;
    uv_fs_stat(fs_loop, fs_req, path, check_default_files_async);
    return;
  } else {
    /* found a regular file */
    res->size_content = fs_req->statbuf.st_size;
    res->path_content = strdup(fs_req->path ? fs_req->path : "<unknown>");
    res->mime_content = match_mime_type(fs_req->path ? fs_req->path : "");
    found_and_sendfs_req(client);
  }

  uv_fs_req_cleanup(fs_req);
  free(fs_req);
}


void file_handle_init(uv_loop_t *loop, const webconfig_t *config)
{
    fs_loop = loop;
    fs_web_config = config;
}

int handle_file_request(client_t *client)
{
  if (!client) return -1;
  if (!fs_loop || !fs_web_config) return -1;

  request_t *req = &client->request;
  response_t *res = &client->response;

  char path[MAX_PATH_LENGTH];
  if (build_safe_path(fs_web_config->www_root, req->url, path, sizeof(path)) != 0) {
    /* security / invalid path */
    send_html_response(client, HTTP_STATUS_NOT_FOUND, getResponse404Content());
    return -1;
  }

  /* start async stat of path */
  uv_fs_t *fs_req = malloc(sizeof(uv_fs_t));
  memset(fs_req, 0, sizeof(*fs_req));
  fs_req->data = client;
  uv_fs_stat(fs_loop, fs_req, path, check_path_async);

  return 0;
}
