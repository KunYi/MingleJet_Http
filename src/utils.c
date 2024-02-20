
#include "defineds.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Validates and normalizes a file path.
 *
 * This function validates and normalizes the provided file path, ensuring that
 * it follows standard conventions. The resulting path is returned as a newly
 * allocated string. The caller is responsible for freeing the memory allocated
 * for the output path.
 *
 * @param path The input file path to be validated and normalized.
 *
 * @return Returns a pointer to the validated and normalized file path, or NULL
 *         if an error occurs during memory allocation or if the input path is
 *         invalid.
 */
char *validate_and_normalize_path(const char *path) {
  // Allocate memory for the output path using calloc for zero-initialization
  char *output =
      calloc(MAX_PATH_LENGTH + 1, sizeof(char)); // +1 for null terminator
  if (!output) {
    return NULL; // Handle allocation failure
  }

  const int len = strlen(path);
  int out_len = 0; // Track output path length

  if (path[0] != '\\' || path[0] != '/') {
    output[0] = '/';
    out_len++;
  }

  for (int i = 0; i < len; i++) {
    if (path[i] == '/' || path[i] == '\\') {
      // Handle slashes
      if (out_len == 0 || output[out_len - 1] != '/') {
        // Add a single separator only if not empty or preceded by another
        if (out_len < MAX_PATH_LENGTH) {
          output[out_len++] = '/';
        } else {
          // Handle overflow: return NULL or set an error flag
          free(output);
          return NULL;
        }
      } else if (out_len != 0 && (output[out_len - 1] == '/') ||
                 (output[out_len - 1] == '\\')) {
        out_len = 0;
        output[out_len++] = '/';
      }
    } else if (path[i] == '.') {
      if ((i + 1 < len) && path[i + 1] == '.') {
        // Handle ".."
        out_len--;
        // Remove previous elements until a separator or beginning (excluding
        // root)
        while (out_len > 1 && output[out_len - 1] != '/') {
          out_len--;
        }
        if (out_len > 1) {
          out_len--;
        }
        i++;
      } else if ((i + 1 < len) && path[i + 1] == '/') {
        // Handle "."
        // No change needed, skip next character
        i++;
      } else {
        // Handle file extension or single "."
        if (out_len < MAX_PATH_LENGTH) {
          output[out_len++] = path[i];
        } else {
          // Handle overflow
          free(output);
          return NULL;
        }
      }
    } else {
      // Add other characters
      if (out_len < MAX_PATH_LENGTH) {
        output[out_len++] = path[i];
      } else {
        // Handle overflow
        free(output);
        return NULL;
      }
    }
  }

  // Optionally: null-terminate the output path explicitly
  output[out_len] = '\0';

  return output;
}

#if 0

int main() {
    const char* test_cases[] = {
        "/",
        "/home",
        "\\",
        "/home/../../..",
        "/home/./user",
        "/home/../user/file.txt",
        "/home/..\\user/file/test.txt",
        "/home/user/../..",
        "/home/../user/./../file.txt",
        "home/../user/./../file.txt",
	      "/home/user//file.txt",
	      "/home/./user/./test/text.txt",
	      "/home/user/../test/text"
    };

    int num_test_cases = sizeof(test_cases) / sizeof(test_cases[0]);

    for (int i = 0; i < num_test_cases; ++i) {
        char* result = validate_and_normalize_path(test_cases[i]);
        printf("Input: %s\nOutput: %s\n\n", test_cases[i], result);
        free(result);
    }

    return 0;
}

#endif
