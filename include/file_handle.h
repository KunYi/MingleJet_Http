#pragma once

#include "webserver.h"

/**
 * @brief Initializes the file handle module.
 *
 * This function sets up the necessary resources and configurations for the file server.
 * It should be called once during the application's startup process.
 *
 * @param loop The active libuv event loop.
 * @param config A pointer to the web server's configuration structure.
 */
void file_handle_init(const uv_loop_t *loop, const webconfig_t *config);

/**
 * @brief Handles an incoming file request.
 *
 * This function is the main entry point for processing requests for static files.
 * It examines the request URL, checks for the corresponding file or directory on the filesystem,
 * and initiates the asynchronous process of sending the file or an appropriate error response.
 *
 * @param client A pointer to the client_t structure representing the connected client.
 * @return Returns 0 on success, or a negative value if an immediate error occurs.
 */
int handle_file_request(client_t *client);
