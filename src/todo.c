#include "todo.h"

// Declare a uthash table to store todo_t structs
static todo_t *todos = NULL;
// Global variable to store the next ID to be assigned
static unsigned int next_id = 0;

// Function to initialize a todo_t struct
todo_t *todo_init(const unsigned int id, const char *title, unsigned int done,
                  const char *body) {
  todo_t *todo = calloc(1, sizeof(todo_t));
  if (!todo) {
    return NULL; // Handle allocation failure
  }

  todo->ID = id;
  todo->Title = strdup(title); // Make a copy of the title
  if (!todo->Title) {
    free(todo); // Free todo if title duplication fails
    return NULL;
  }
  todo->Done = done;
  todo->Body = strdup(body); // Make a copy of the body
  if (!todo->Body) {
    free(todo->Title); // Free title if body duplication fails
    free(todo);
    return NULL;
  }

  return todo;
}

// Function to destroy a todo_t struct
void todo_destroy(todo_t *todo) {
  if (todo) {
    free(todo->Title);
    free(todo->Body);
    free(todo);
  }
}

// Function to store a todo_t struct in a JSON object using Jansson
json_t *todo_to_json(todo_t *todo) {
  if (!todo) {
    return NULL; // Handle NULL input
  }

  json_t *json_todo = json_object();
  if (!json_todo) {
    return NULL; // Handle allocation failure
  }

  json_object_set_new(json_todo, "id", json_integer(todo->ID));
  json_object_set_new(json_todo, "title", json_string(todo->Title));
  json_object_set_new(json_todo, "done", json_integer(todo->Done));
  json_object_set_new(json_todo, "body", json_string(todo->Body));

  return json_todo;
}

todo_t *create_todo_from_json(json_t *json_todo) {
  if (!json_is_object(json_todo)) {
    return NULL; // Handle invalid JSON type
  }

  json_t *title_json = json_object_get(json_todo, "title");
  if (!json_is_string(title_json)) {
    return NULL; // Handle invalid "Title" type
  }
  const char *title = json_string_value(title_json);

  json_t *body_json = json_object_get(json_todo, "body");
  if (!json_is_string(body_json)) {
    return NULL; // Handle invalid "Body" type
  }
  const char *body = json_string_value(body_json);

  return todo_init(++next_id, title, 0, body);
}

// Function to convert a JSON object to a todo_t struct using Jansson
todo_t *todo_from_json(json_t *json_todo) {
  if (!json_is_object(json_todo)) {
    return NULL; // Handle invalid JSON type
  }

  json_t *id_json = json_object_get(json_todo, "id");
  if (!json_is_integer(id_json)) {
    return NULL; // Handle invalid "ID" type
  }
  unsigned int id = json_integer_value(id_json);

  json_t *title_json = json_object_get(json_todo, "title");
  if (!json_is_string(title_json)) {
    return NULL; // Handle invalid "Title" type
  }
  const char *title = json_string_value(title_json);

  json_t *done_json = json_object_get(json_todo, "done");
  if (!json_is_integer(done_json)) {
    return NULL; // Handle invalid "Done" type
  }
  unsigned int done = json_integer_value(done_json);

  json_t *body_json = json_object_get(json_todo, "body");
  if (!json_is_string(body_json)) {
    return NULL; // Handle invalid "Body" type
  }
  const char *body = json_string_value(body_json);

  return todo_init(id, title, done,
                   body); // Create a new todo_t from extracted values
}

/**
 * @brief Adds a new `todo_t` struct to the `todos` uthash table.
 *
 * Creates a new `todo_t` struct with the given attributes and a unique ID,
 * and inserts it into the `todos` uthash table using the ID as the key.
 *
 * @param title The title of the new todo.
 * @param done Whether the new todo is completed (1) or not (0).
 * @param body The description or details of the new todo.
 *
 * @return 0 on success, -1 on failure (e.g., allocation failure).
 *
 * @details
 *  - This function increments a global counter `next_id`
 *    to assign a unique ID to the new todo.
 *  - The `todo_init` function is used to create the new `todo_t` struct
 *    with the provided attributes and the generated ID.
 *  - If allocation fails during `todo_init`, the function returns -1.
 *  - The `HASH_ADD_INT` macro from uthash inserts the new `todo_t` struct
 *    into the `todos` table using the `ID` field as the key.
 *  - On success, the function returns 0.
 */
int todo_add(const char *title, unsigned int done, const char *body) {
  const unsigned id = ++next_id;
  todo_t *todo = todo_init(id, title, done, body);
  if (!todo) {
    return -1;
  }

  HASH_ADD_INT(todos, ID, todo);

  return 0;
}

int todos_append(todo_t *todo) {
  if (!todo) {
    return -1;
  }

  HASH_ADD_INT(todos, ID, todo);
  return 0;
}

// Deletes a `todo_t` struct from the uthash table.
int todo_delete(const unsigned int id) {
  todo_t *todo;
  HASH_FIND_INT(todos, &id, todo);
  if (!todo) {
    return -1;
  }

  HASH_DEL(todos, todo);
  todo_destroy(todo);

  return 0;
}

// Finds a `todo_t` struct in the uthash table by its ID.
todo_t *todo_find(const unsigned int id) {
  todo_t *todo;
  HASH_FIND_INT(todos, &id, todo);
  return todo;
}

// Updates an existing `todo_t` struct in the `todos` uthash table.
int todo_update(const unsigned int id, const char *title, unsigned int done,
                const char *body) {
  todo_t *todo = todo_find(id);
  if (!todo) {
    return -1; // Todo not found
  }

  // Update fields (free and reallocate memory if needed)
  if (title) {
    free(todo->Title);
    todo->Title = strdup(title);
    if (!todo->Title) {
      return -1; // Memory allocation failure
    }
  }
  todo->Done = done;
  if (body) {
    free(todo->Body);
    todo->Body = strdup(body);
    if (!todo->Body) {
      return -1; // Memory allocation failure
    }
  }

  // Optional: Re-insert the updated todo
  // HASH_DEL(todos, todo);
  // HASH_ADD_INT(todos, ID, todo);
  return 0; // Success
}

char *todos_dump(void) {
  json_t *todos_array = json_array();
  if (!todos_array) {
    return NULL;
  }

  todo_t *current, *tmp;
  HASH_ITER(hh, todos, current, tmp) {
    json_t *todo =
        json_pack("{s:i,s:s,s:i,s:s}", "id", current->ID, "title",
                  current->Title, "done", current->Done, "body", current->Body);
    if (!todo) {
      json_decref(todos_array); // Free memory on allocation failure
      return NULL;
    }
    json_array_append_new(todos_array, todo);
  }

  char *jsonstr = json_dumps(todos_array, JSON_COMPACT);

  json_decref(todos_array); // Free memory
  return jsonstr;
}
