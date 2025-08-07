#pragma once

#include "client_manager.h"
#include "llhttp.h"
#include <stddef.h>
#include "uv.h"

const char *match_mime_type(const char *path);

uv_buf_t make_response_header(int status, response_t *res);

void send_text_response(client_t *client, const llhttp_status_t code,
                               const char *content);
void send_html_response(client_t *client, const llhttp_status_t code,
                               const char *content);
