#pragma once
#include "webserver.h"
#include <uthash.h>

typedef int (*route_handler_t)(client_t *client);

typedef struct route_s {
    char *path;                // Route path (e.g., "/index.html")
    route_handler_t handler;    // Handler function for the route
    UT_hash_handle hh;         // uthash handle for hash table
} route_t;

void router_init(void);
void router_register(const char *path, route_handler_t handler);
void router_dispatch(client_t *client);
void router_cleanup(void);
