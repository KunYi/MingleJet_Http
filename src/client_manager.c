

#include "client_manager.h"
#include "defineds.h"
#include "http_parser.h"
#include "router.h"
#include "utils.h"
#include "utlist.h"

// #include "router.h"


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



client_t *client_create(uv_loop_t *loop) {
    client_t *client = calloc(1, sizeof(client_t));
    if (client) {
        uv_tcp_init(loop, &client->handle);
        client->handle.data = client;
        client->parser = malloc(sizeof(http_parser_t));
        ((http_parser_t *)client->parser)->parser.data = client;
        http_parser_init(client->parser, client);
        // Add to active client list
        CLIENT_SET_IN_USE(client);
        LL_APPEND(activeClientList, client);
    }
    return client;
}


void release_client(client_t *client) {
    if (client) {
        client_clear_in_use(client);
        if (client_is_flags_free(client)) {
            LL_DELETE(activeClientList, client);
            free_client(client);
        }
    }
}

void client_free(client_t *client) {
    if (client->request.url) free(client->request.url);
    if (client->request.body) free(client->request.body);
    if (client->request.query_param) utarray_free(client->request.query_param);
    http_parser_free(client->parser);
    free(client->parser);
    free(client);
}

void cleanup_resources(void) {
  client_t *elt, *tmp;
  LL_FOREACH_SAFE(activeClientList, elt, tmp) {
    LL_DELETE(activeClientList, elt);
    free_client(elt);
  }
}
void client_handle_request(client_t *client, const char *data, size_t len) {
    http_parser_t *parser = client->parser;
    if (http_parser_execute(parser, data, len) == HPE_OK) {
        router_process(client);
    }
}
