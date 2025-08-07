#pragma once
#include "client_manager.h"
#include <llhttp.h>

typedef struct get_param_s {
  char *name;
  char *value;
} get_param_t;

typedef struct {
  llhttp_t parser;
  llhttp_settings_t settings;
} http_parser_t;


void http_parser_init(http_parser_t *parser, client_t *client);
int http_parser_execute(http_parser_t *parser, const char *data, size_t len);
void http_parser_free(http_parser_t *parser);
