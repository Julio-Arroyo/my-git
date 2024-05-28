#ifndef ADD_H
#define ADD_H

#include <stddef.h>

#define ERR_STR_HEADER "ERROR add_files():"

// stage changes to files to index, add object to store
void add_files(const char **file_paths, size_t file_count);

#endif
