#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "constants.h"
#include "linked_list.h"

uint8_t from_octal(char c);
uint8_t from_decimal(char c);
uint8_t from_hex(char c);

char to_hex(uint8_t value);

void hash_to_hex(const uint8_t hash_bytes[HASH_BYTES], object_hash_t hash_string);

/**
 * Populates 'hash_bytes'
 */
void hex_to_hash(const object_hash_t hash_string, uint8_t hash_bytes[HASH_BYTES]);

// Reads a big-endian unsigned integer with the given number of bytes
size_t read_be(const uint8_t *bytes, size_t length);

// Writes a big-endian unsigned integer with the given number of bytes
void write_be(size_t value, uint8_t *bytes, size_t length);

bool starts_with(const char *string, const char *prefix);

void make_dirs(char *path);
void make_parent_dirs(char *path);

size_t get_file_size(FILE *f);

/*
 * Populates 'files' with the relative path names of all files under
 * 'root_dir'. If 'recursive' is true, then files in subdirectories are
 * also added
 */
void get_all_files(const char *dir, linked_list_t *files, bool recursive);

#endif // #ifndef UTIL_H
