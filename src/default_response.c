#include "default_response.h"

static const char *res404content =
  "<!DOCTYPE html>"
  "<html>"
  "<header>"
  "<title>MingleJet</title>"
  "</header>"
  "<body>"
  "<H1>Not Found</H1>"
  "</body>"
  "</html>";

static const char *res500content =
  "<!DOCTYPE html>"
  "<html>"
  "<header>"
  "<title>MingleJet</title>"
  "</header>"
  "<body>"
  "<h1>500 Internal Server Error</h1>"
  "<p>An unexpected error occurred while processing your request.</p>"
  "<p>Please try again later.</p>"
  "</body>"
  "</html>";

const char *getResponse404Content(void) {
  return res404content;
}

const char *getResponse500Content(void) {
  return res500content;
}
