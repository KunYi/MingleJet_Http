#include "defineds.h"
#include "http_parser.h"
#include "utarray.h"
#include "utils.h"

static void get_param_cleanup(get_param_t *param) {
  free(param->name);
  free(param->value);
}

static UT_icd get_params_icd = {sizeof(get_param_t), NULL, NULL,
                                (void (*)(void *))get_param_cleanup};


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
static int on_url(llhttp_t *parser, const char *at, size_t length) {
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
static int on_status(llhttp_t *parser, const char *at, size_t length) {
  UNUSED(parser);
  printf("HTTP version: %.*s\n", (int)length, at);
  return 0;
}

// Callback to handle header field
static int on_header_field(llhttp_t *parser, const char *at, size_t length) {
  UNUSED(parser);
  // printf("Header field: %.*s\n", (int)length, at);
  return 0;
}

// Callback to handle header value
static int on_header_value(llhttp_t *parser, const char *at, size_t length) {
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

void http_parser_init(http_parser_t *parser, client_t *client) {
  // Initialize HTTP parser settings
  llhttp_settings_init(&parser->settings);
  parser->settings.on_message_begin = on_message_begin;
  parser->settings.on_message_complete = on_message_complete;
  parser->settings.on_url = on_url;
  parser->settings.on_status = on_status;
  parser->settings.on_header_field = on_header_field;
  parser->settings.on_header_value = on_header_value;
  parser->settings.on_headers_complete = on_headers_complete;
  parser->settings.on_body = on_body;
  // Set other callbacks
  llhttp_init(&parser->parser, HTTP_REQUEST, &parser->settings);
  parser->parser.data = client;
}
int http_parser_execute(http_parser_t *parser, const char *data, size_t len) {
    return llhttp_execute(&parser->parser, data, len);
}

void http_parser_free(http_parser_t *parser) {
    // No additional cleanup needed for llhttp
}
