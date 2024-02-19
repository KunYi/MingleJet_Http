#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool validate_path(const char *path) {
  // Check for empty path
  if (!path || !*path) {
    return false;
  }

  char path_copy[strlen(path) + 1];
  strcpy(path_copy, path);

  // Start from the root
  char current_path[strlen(path) + 1];
  strcpy(current_path, "/");

  // Split the path by '/'
  char *component = strtok(path_copy, "/");
  while (component) {
    // Check if component is valid
    if (strcmp(component, "..") == 0) {
      // Go up one level, but don't allow going above the root
      if (strlen(current_path) > 1) {
        // Remove the last directory from current_path
        char *last_slash = strrchr(current_path, '/');
        *last_slash = '\0';
      } else {
        // Trying to go above the root, invalid path
	printf("result:%s\n", current_path);
        return false;
      }
    } else if (strcmp(component, ".") == 0) {
      // Stay in the same directory (no change)
    } else {
      // Append the component to current_path
      strcat(current_path, "/");
      strcat(current_path, component);
    }

    // Move to the next component
    component = strtok(NULL, "/");
  }

  // Check if the final path is valid (not above the root)
  printf("result:%s\n", current_path);
  return strlen(current_path) >= 1;
}

#if 0
void test_validate_path() {
  // Test cases with expected results
  struct test_case {
    const char *path;
    bool expected_result;
  } test_cases[] = {
    {"/", true},
    {"/home", true},
    {"/home/user", true},
    {"../", false},
    {"../../", false},
    {"../home", false},
    {"/home/..", true},
    {"/home/./user", true},
    {"/home//user", true},
    {"/home/user/./file.txt", true},
    {"/home/user/../file.txt", true},
    {"", false},
    {NULL, false},
  };

  // Run each test case
  for (int i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
    const char *path = test_cases[i].path;
    bool expected_result = test_cases[i].expected_result;
    bool actual_result = validate_path(path);

    // Print test case and result
    printf("Test case: %s - Expected: %d, Actual: %d\n\n", path, expected_result, actual_result);

    // Assert if results don't match
    if (actual_result != expected_result) {
      printf("Test case failed!\n");
      exit(1); // Indicate test failure
    }
  }

  printf("All test cases passed!\n");
}

int main() {
  test_validate_path();
  return 0;
}
#endif
