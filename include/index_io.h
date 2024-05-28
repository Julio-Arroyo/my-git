#ifndef INDEX_IO_H
#define INDEX_IO_H

#include <sys/stat.h>
#include <stdint.h>
#include <time.h>
#include "hash_table.h"
#include "constants.h"
#include "object_io.h"

#define SUPPORTED_VERSION 2
#define UNUSED 0

typedef struct index_entry {
    uint32_t size;
    object_hash_t sha1;
    char *fname;
    uint32_t fname_length;
    time_t mtime;
    uint32_t mode;  // split into 16-bits unused, 4-bits (0b1000 for "regular file"), 3-bits unused, 9-bits UNIX permissions
    bool removed;
} index_entry_t;

typedef struct index_file {
    // Maps filenames to their index_entry
    hash_table_t *entries;
} index_file_t;

index_file_t *empty_index_file(void);
index_file_t *read_index_file(void);

void write_index_file(const index_file_t*);

void free_index_file(index_file_t *);
void free_index_entry(index_entry_t *);

bool index_is_empty(index_file_t *);
bool index_contains_file(index_file_t *, const char *);

index_entry_t *index_create_entry(const char *fname,
                                  struct stat *sb,
                                  object_hash_t sha1);

#endif
