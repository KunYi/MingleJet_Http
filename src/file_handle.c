#include "client_manager.h"
#include "file_handle.h"
#include "default_response.h"
#include "defineds.h"
// #include "response.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

// Static variables to hold the loop and config
static const uv_loop_t *fs_loop;
static const webconfig_t *fs_web_config;

void file_handle_init(const uv_loop_t *loop, const webconfig_t *config)
{
    fs_loop = loop;
    fs_web_config = config;
}













int handle_file_request(client_t *client)
{

}
