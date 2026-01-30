#ifndef _XHT_H_
#define _XHT_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * This library provides two types of hash tables:
 *
 * 1. A simple one-way hash table (xht_t) with two key variations:
 *    - 64-bit integers (for XIDs, pointers, etc.)
 *    - Null-terminated strings
 *
 * 2. A specialized two-way hash table (xht_2way_t) for mapping
 *    strings to 64-bit integer IDs and vice-versa, intended for
 *    use with atom tables.
 */

typedef struct xht_table xht_t;
typedef struct xht_2way_table xht_2way_t;

/* --- One-Way Integer-keyed Hashtable --- */

xht_t *xht_create_int_table(uint32_t initial_size);

bool xht_set_int(xht_t *table, uint64_t key, void *data);
void *xht_get_int(xht_t *table, uint64_t key);
bool xht_delete_int(xht_t *table, uint64_t key);
void xht_iter_int(xht_t *table, void (*func)(uint64_t key, void *data, void *user_data), void *user_data);
void xht_destroy_int_table(xht_t *table);


/* --- One-Way String-keyed Hashtable --- */

/* const_keys: if true, keys are not copied and must remain valid for the
 * lifetime of the table. If false, keys are copied.
 */
xht_t *xht_create_string_table(uint32_t initial_size, bool const_keys);

bool xht_set_string(xht_t *table, const char *key, void *data);
void *xht_get_string(xht_t *table, const char *key);
bool xht_delete_string(xht_t *table, const char *key);
void xht_iter_string(xht_t *table, void (*func)(const char *key, void *data, void *user_data), void *user_data);
void xht_destroy_string_table(xht_t *table);


/* --- Two-Way Atom Table (String <-> Integer ID) --- */

xht_2way_t *xht_atom_table_create(uint32_t initial_size);
void xht_atom_table_destroy(xht_2way_t *table);

/* The caller is responsible for generating unique IDs.
 * The string key is always copied.
 */
bool xht_atom_table_set(xht_2way_t *table, const char *string, uint64_t id);

uint64_t xht_atom_table_get_id(xht_2way_t *table, const char *string);
const char *xht_atom_table_get_string(xht_2way_t *table, uint64_t id);

bool xht_atom_table_delete_string(xht_2way_t *table, const char *string);
bool xht_atom_table_delete_id(xht_2way_t *table, uint64_t id);

void xht_atom_table_iter(xht_2way_t *table, void (*func)(const char *string, uint64_t id, void *user_data), void *user_data);


#endif /* _XHT_H_ */
