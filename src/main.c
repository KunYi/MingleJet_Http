#include "defineds.h"
#include "webserver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

int main() {
  webconfig_t *webconfig;
  const uint32_t defaluts_files = 2;
  webconfig = malloc(sizeof(webconfig_t) + (sizeof(char *) * defaluts_files));
  webconfig->host = "0.0.0.0";
  webconfig->port = 8080;
  webconfig->www_root = "./dist";
  webconfig->def_cnt = defaluts_files;
  webconfig->defaults[0] = "index.html";
  webconfig->defaults[1] = "index.htm";
  webconfig->cors = false;
  uv_loop_t *def_loop = uv_default_loop();
  int ret = webserver(def_loop, webconfig);
  free(webconfig);
  uv_loop_close(def_loop);

  fprintf(stdout, "\nexit program %d\n", ret);
  return ret;
}
