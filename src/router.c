#include "file_handle.h"
#include "router.h"
#include <string.h>
#include <stdlib.h>

static route_t *routes = NULL; // Head of the hash table

void router_init(void) {
    // Register default routes
    router_register("/", handle_file_request);
    router_register("/index.html", handle_file_request);
}

void router_register(const char *path, route_handler_t handler) {
    route_t *route;

    // Check if route already exists
    HASH_FIND_STR(routes, path, route);
    if (route) {
        // Update existing route
        route->handler = handler;
        return;
    }

    // Create new route
    route = malloc(sizeof(route_t));
    route->path = strdup(path);
    route->handler = handler;

    // Add to hash table
    HASH_ADD_STR(routes, path, route);
}

void router_dispatch(client_t *client) {
    route_t *route;

    // Look up route in hash table
    HASH_FIND_STR(routes, client->request.url, route);
    if (route) {
        // Call the registered handler
        route->handler(client);
    } else {
        // Default to file handler for unmatched routes
        // handle_file_request(client);
    }
}

void router_cleanup(void) {
    route_t *route, *tmp;

    // Iterate and free all routes
    HASH_ITER(hh, routes, route, tmp) {
        HASH_DEL(routes, route);
        free(route->path);
        free(route);
    }
}
