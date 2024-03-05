#pragma once
#include "uthash.h"
#include <jansson.h>
#include <string.h>

typedef struct todo_s {
  unsigned int ID; /* key */
  char *Title;
  unsigned int Done;
  char *Body;
  UT_hash_handle hh;
} todo_t;

/**
 * @brief Initializes a `todo_t` struct with given attributes.
 *
 * Allocates memory for the struct and its `Title` and `Body` members.
 * Makes copies of the provided `title` and `body` strings using `strdup`.
 * Returns a pointer to the newly created `todo_t` struct, or `NULL` on failure.
 *
 * @param id The unique identifier for the todo (used as the key).
 * @param title The title of the todo.
 * @param done Indicates whether the todo is completed (1) or not (0).
 * @param body The description or details of the todo.
 *
 * @return A pointer to the newly created `todo_t` struct, or `NULL` on failure.
 */
todo_t *todo_init(const unsigned int id, const char *title, unsigned int done,
                  const char *body);

/**
 * @brief Deallocates the memory associated with a `todo_t` struct.
 *
 * Frees the memory allocated for the struct itself, `Title`, and `Body`.
 * It is safe to call this function for `NULL` pointers.
 *
 * @param todo The `todo_t` struct to destroy.
 */
void todo_destroy(todo_t *todo);

/**
 * @brief Creates a JSON object representation of a `todo_t` struct.
 *
 * Uses Jansson library to create a JSON object with member key-value pairs
 * corresponding to the attributes of the `todo_t` struct.
 * Returns a pointer to the newly created JSON object, or `NULL` on failure.
 *
 * @param todo The `todo_t` struct to convert to JSON.
 *
 * @return A pointer to the newly created JSON object, or `NULL` on failure.
 */
json_t *todo_to_json(todo_t *todo);

/**
 * @brief Creates a `todo_t` struct from a JSON object representation.
 *
 * Uses Jansson library to parse the JSON object and extract the values of the
 * "ID", "Title", "Done", and "Body" members.
 * Creates and returns a new `todo_t` struct initialized with the extracted
 * values, or `NULL` on failure.
 *
 * @param json_todo The JSON object representing a todo.
 *
 * @return A pointer to the newly created `todo_t` struct, or `NULL` on failure.
 */
todo_t *todo_from_json(json_t *json_todo);

/**
 * @brief Creates a `todo_t` object from a JSON representation.
 *
 * This function parses a JSON object and extracts the necessary information to
 * create a new `todo_t` object. The expected JSON format is:
 *
 * ```json
 * {
 *   "Title": "string",
 *   "Body": "string"
 * }
 * ```
 *
 * @param json_todo A pointer to the JSON object representing the todo.
 *
 * @return A pointer to the newly created `todo_t` object, or NULL if the JSON
 * is invalid or fails to provide the required fields.
 *
 * @throws This function throws no exceptions. However, it returns NULL to
 * indicate potential errors:
 *         - If the provided `json_todo` is not a JSON object.
 *         - If the "Title" field is missing or not a string.
 *         - If the "Body" field is missing or not a string.
 */
todo_t *create_todo_from_json(json_t *json_todo);

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
 */
int todo_add(const char *title, unsigned int done, const char *body);

/**
 * @brief Appends a `todo_t` object to the global todos hash table.
 *
 * This function adds a given `todo_t` object to the global `todos` hash table,
 * keyed by its ID. The hash table is implemented using uthash.
 *
 * @param todo A pointer to the `todo_t` object to be added.
 *
 * @return
 *   - 0 on success (object added successfully).
 *   - -1 on failure (e.g., invalid `todo` object or insertion error).
 *
 * @pre The `todo_t` object must be valid and have a unique ID.
 * @post The `todo_t` object is added to the `todos` hash table, or the function
 * returns an error.
 *
 * @note
 *   - This function modifies the global `todos` hash table. Callers should
 * ensure thread safety if accessing the hash table concurrently.
 *   - The function assumes that the memory for the `todo` object has been
 * allocated and will be managed by the caller.
 */
int todos_append(todo_t *todo);

/**
 * @brief Deletes a `todo_t` struct from the uthash table.
 *
 * Finds the `todo_t` struct with the given ID in the `todos` table
 * and removes it. Also calls `todo_destroy` to free the associated memory.
 * Returns 0 on success, or -1 on failure.
 *
 * @param id The unique identifier of the todo to delete.
 *
 * @return 0 on success, -1 on failure.
 */
int todo_delete(const unsigned int id);

/**
 * @brief Finds a `todo_t` struct in the uthash table by its ID.
 *
 * Searches the `todos` table for a `todo_t` struct with the given ID.
 * Returns the found struct, or `NULL` if not found.
 *
 * @param id The unique identifier of the todo to find.
 *
 * @return A pointer to the found `todo_t` struct, or `NULL` if not found.
 */
todo_t *todo_find(const unsigned int id);

/**
 * @brief Updates an existing `todo_t` struct in the `todos` uthash table.
 *
 * Finds the `todo_t` struct with the given ID and updates its attributes
 * with the provided values (optionally). Updates can include title, completion
 * status, and body.
 *
 * @param id The unique identifier of the todo to update.
 * @param title The new title of the todo (optional, NULL to keep existing).
 * @param done The new completion status (1 for done, 0 for pending, optional).
 * @param body The new description of the todo (optional, NULL to keep
 * existing).
 *
 * @return 0 on success, -1 on failure (todo not found or memory allocation
 * issue).
 */
int todo_update(const unsigned int id, const char *title, unsigned int done,
                const char *body);

/**
 * @brief Creates a JSON string representation of all todos in the hash table.
 *
 * This function iterates through the global `todos` hash table and creates a
 * JSON array containing an object for each `todo_t` element. The object
 * includes key-value pairs for "ID", "Title", "Done", and "Body" attributes.
 *
 * @return A dynamically allocated string containing the JSON representation
 *         of all todos, or NULL on failure (e.g., memory allocation issues).
 *
 * @note
 *   - The returned string must be freed by the caller using `free`.
 *   - This function modifies the global `todos` hash table by iterating through
 * it. Callers should ensure thread safety if accessing the hash table
 * concurrently.
 */
char *todos_dump(void);
