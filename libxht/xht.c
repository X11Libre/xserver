#include "xht.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define XHT_LOAD_FACTOR 0.75

typedef struct xht_entry {
    void *key;
    void *data;
    struct xht_entry *next;
} xht_entry_t;

struct xht_table {
    uint32_t size;
    uint32_t count;
    xht_entry_t **entries;
    bool const_keys; // Only for string tables
};

struct xht_2way_table {
    xht_t *string_to_id;
    xht_t *id_to_string;
};

static uint32_t hash_int(uint64_t key)
{
    key = (~key) + (key << 21); // key = (key << 21) - key - 1;
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8); // key * 265
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4); // key * 21
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return (uint32_t)key;
}

static uint32_t hash_string(const char *str)
{
    uint32_t hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

static void xht_resize(xht_t *table, uint32_t new_size, bool is_int_table);

/* --- Generic one-way table implementation --- */

static xht_t *xht_create_internal(uint32_t initial_size, bool const_keys) {
    xht_t *table = calloc(1, sizeof(xht_t));
    if (!table) return NULL;

    table->entries = calloc(initial_size, sizeof(xht_entry_t *));
    if (!table->entries) {
        free(table);
        return NULL;
    }
    table->size = initial_size;
    table->const_keys = const_keys;
    return table;
}

static void xht_destroy_internal(xht_t *table, bool is_int_table) {
    if (!table) return;
    for (uint32_t i = 0; i < table->size; i++) {
        xht_entry_t *entry = table->entries[i];
        while (entry) {
            xht_entry_t *next = entry->next;
            if (!is_int_table && !table->const_keys) {
                free(entry->key);
            }
            free(entry);
            entry = next;
        }
    }
    free(table->entries);
    free(table);
}

/* --- One-Way Integer-keyed Hashtable --- */

xht_t *xht_create_int_table(uint32_t initial_size) {
    return xht_create_internal(initial_size, true); // const_keys is irrelevant for ints
}

void xht_destroy_int_table(xht_t *table) {
    xht_destroy_internal(table, true);
}

bool xht_set_int(xht_t *table, uint64_t key, void *data) {
    if (table->count >= table->size * XHT_LOAD_FACTOR) {
        xht_resize(table, table->size * 2, true);
    }

    uint32_t index = hash_int(key) % table->size;
    xht_entry_t *entry = table->entries[index];

    while (entry) {
        if ((uint64_t)(uintptr_t)entry->key == key) {
            entry->data = data;
            return true;
        }
        entry = entry->next;
    }

    xht_entry_t *new_entry = calloc(1, sizeof(xht_entry_t));
    if (!new_entry) return false;

    new_entry->key = (void *)(uintptr_t)key;
    new_entry->data = data;
    new_entry->next = table->entries[index];
    table->entries[index] = new_entry;
    table->count++;
    return true;
}

void *xht_get_int(xht_t *table, uint64_t key) {
    uint32_t index = hash_int(key) % table->size;
    xht_entry_t *entry = table->entries[index];
    while (entry) {
        if ((uint64_t)(uintptr_t)entry->key == key) {
            return entry->data;
        }
        entry = entry->next;
    }
    return NULL;
}

bool xht_delete_int(xht_t *table, uint64_t key) {
    uint32_t index = hash_int(key) % table->size;
    xht_entry_t *entry = table->entries[index];
    xht_entry_t *prev = NULL;

    while (entry) {
        if ((uint64_t)(uintptr_t)entry->key == key) {
            if (prev) {
                prev->next = entry->next;
            } else {
                table->entries[index] = entry->next;
            }
            free(entry);
            table->count--;
            return true;
        }
        prev = entry;
        entry = entry->next;
    }
    return false;
}

void xht_iter_int(xht_t *table, void (*func)(uint64_t key, void *data, void *user_data), void *user_data) {
    for (uint32_t i = 0; i < table->size; i++) {
        for (xht_entry_t *entry = table->entries[i]; entry; entry = entry->next) {
            func((uint64_t)(uintptr_t)entry->key, entry->data, user_data);
        }
    }
}


/* --- One-Way String-keyed Hashtable --- */

xht_t *xht_create_string_table(uint32_t initial_size, bool const_keys) {
    return xht_create_internal(initial_size, const_keys);
}

void xht_destroy_string_table(xht_t *table) {
    xht_destroy_internal(table, false);
}

static xht_entry_t* xht_find_string_entry(xht_t *table, const char *key) {
    uint32_t index = hash_string(key) % table->size;
    xht_entry_t *entry = table->entries[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

bool xht_set_string(xht_t *table, const char *key, void *data) {
    if (table->count >= table->size * XHT_LOAD_FACTOR) {
        xht_resize(table, table->size * 2, false);
    }

    xht_entry_t *entry = xht_find_string_entry(table, key);
    if (entry) {
        entry->data = data;
        return true;
    }

    xht_entry_t *new_entry = calloc(1, sizeof(xht_entry_t));
    if (!new_entry) return false;

    if (!table->const_keys) {
        new_entry->key = strdup(key);
        if (!new_entry->key) {
            free(new_entry);
            return false;
        }
    } else {
        new_entry->key = (void *)key;
    }

    new_entry->data = data;
    uint32_t index = hash_string(key) % table->size;
    new_entry->next = table->entries[index];
    table->entries[index] = new_entry;
    table->count++;
    return true;
}

void *xht_get_string(xht_t *table, const char *key) {
    xht_entry_t *entry = xht_find_string_entry(table, key);
    return entry ? entry->data : NULL;
}

bool xht_delete_string(xht_t *table, const char *key) {
    uint32_t index = hash_string(key) % table->size;
    xht_entry_t *entry = table->entries[index];
    xht_entry_t *prev = NULL;

    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                table->entries[index] = entry->next;
            }
            if (!table->const_keys) {
                free(entry->key);
            }
            free(entry);
            table->count--;
            return true;
        }
        prev = entry;
        entry = entry->next;
    }
    return false;
}

void xht_iter_string(xht_t *table, void (*func)(const char *key, void *data, void *user_data), void *user_data) {
    for (uint32_t i = 0; i < table->size; i++) {
        for (xht_entry_t *entry = table->entries[i]; entry; entry = entry->next) {
            func(entry->key, entry->data, user_data);
        }
    }
}

/* --- Resize Implementation --- */

static void xht_resize(xht_t *table, uint32_t new_size, bool is_int_table) {
    xht_entry_t **old_entries = table->entries;
    uint32_t old_size = table->size;

    xht_entry_t **new_entries_ptr = calloc(new_size, sizeof(xht_entry_t *));
    if (!new_entries_ptr) return;

    table->entries = new_entries_ptr;
    table->size = new_size;
    table->count = 0;

    for (uint32_t i = 0; i < old_size; i++) {
        xht_entry_t *entry = old_entries[i];
        while (entry) {
            xht_entry_t *next = entry->next;
            if (is_int_table) {
                xht_set_int(table, (uint64_t)(uintptr_t)entry->key, entry->data);
            } else {
                bool original_const_keys = table->const_keys;
                table->const_keys = true; // Avoid re-copying keys
                xht_set_string(table, entry->key, entry->data);
                table->const_keys = original_const_keys;
            }
            free(entry);
            entry = next;
        }
    }
    free(old_entries);
}


/* --- Two-Way Atom Table (String <-> Integer ID) --- */

xht_2way_t *xht_atom_table_create(uint32_t initial_size) {
    xht_2way_t *table = calloc(1, sizeof(xht_2way_t));
    if (!table) return NULL;

    table->string_to_id = xht_create_string_table(initial_size, false);
    table->id_to_string = xht_create_int_table(initial_size);

    if (!table->string_to_id || !table->id_to_string) {
        xht_destroy_string_table(table->string_to_id);
        xht_destroy_int_table(table->id_to_string);
        free(table);
        return NULL;
    }
    return table;
}

void xht_atom_table_destroy(xht_2way_t *table) {
    if (!table) return;
    xht_destroy_string_table(table->string_to_id);
    xht_destroy_int_table(table->id_to_string);
    free(table);
}

bool xht_atom_table_set(xht_2way_t *table, const char *string, uint64_t id) {
    if (xht_get_string(table->string_to_id, string) || xht_get_int(table->id_to_string, id)) {
        return false;
    }

    if (!xht_set_string(table->string_to_id, string, (void *)(uintptr_t)id)) {
        return false;
    }

    xht_entry_t *entry = xht_find_string_entry(table->string_to_id, string);
    if (!entry) { // Should not happen
        xht_delete_string(table->string_to_id, string);
        return false;
    }

    if (!xht_set_int(table->id_to_string, id, entry->key)) {
        xht_delete_string(table->string_to_id, string);
        return false;
    }

    return true;
}

uint64_t xht_atom_table_get_id(xht_2way_t *table, const char *string) {
    void *data = xht_get_string(table->string_to_id, string);
    return (uint64_t)(uintptr_t)data;
}

const char *xht_atom_table_get_string(xht_2way_t *table, uint64_t id) {
    return xht_get_int(table->id_to_string, id);
}

bool xht_atom_table_delete_string(xht_2way_t *table, const char *string) {
    uint64_t id = xht_atom_table_get_id(table, string);
    if (id == 0) return false;

    xht_delete_int(table->id_to_string, id);
    xht_delete_string(table->string_to_id, string);
    return true;
}

bool xht_atom_table_delete_id(xht_2way_t *table, uint64_t id) {
    const char *string = xht_atom_table_get_string(table, id);
    if (!string) return false;

    xht_delete_int(table->id_to_string, id);
    xht_delete_string(table->string_to_id, string);
    return true;
}

void xht_atom_table_iter(xht_2way_t *table, void (*func)(const char *string, uint64_t id, void *user_data), void *user_data) {
    xht_iter_string(table->string_to_id, (void (*)(const char *, void *, void *))func, user_data);
}
