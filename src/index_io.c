/*
 * The index file is used as a staging area.
 * Reading from the index file is necessary during a checkout; if
 * a checkout would overwrite a staged change, the checkout should
 * fail. Additionally, the index file has to be overwritten after
 * the checkout.
 */

#include "index_io.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "util.h"
#include "object_io.h"
#include "linked_list.h"

/* Expected file signature for index files */
const char DIRC_SIG[4] = {'D', 'I', 'R', 'C'};

const char *INDEX_PATH = ".git/index";

/*
 * Read a big-endian bytestream
 * (can't just fread() on a little endian system)
 */
uint32_t read32(FILE *f) {
    uint8_t bytes[4];
    fread(bytes, 1, 4, f);
    return read_be(bytes, 4);
}

/*
 * Write 32-bit number 'n' to file 'f' in big-endian order
 */
void write32(FILE *f, uint32_t n) {
    uint8_t bytes[4];
    write_be(n, bytes, 4);
    fwrite(bytes, 1, 4, f);
}

void write16(FILE *f, uint16_t n) {
    uint8_t bytes[2];
    write_be(n, bytes, 2);
    fwrite(bytes, 1, 2, f);
}

index_entry_t *read_index_entry(FILE *f, uint32_t version) {
    index_entry_t *entry = (index_entry_t *) malloc(sizeof(index_entry_t));
    /* Skip over metadata that I don't need */
    fseek(f, 8, SEEK_CUR);
    entry->mtime = read32(f);
    fseek(f, 12, SEEK_CUR);
    entry->mode = read32(f);
    fseek(f, 8, SEEK_CUR);

    entry->size = read32(f);

    uint8_t sha1[HASH_BYTES];
    fread(sha1, 1, HASH_BYTES, f);
    hash_to_hex(sha1, entry->sha1);

    char flags[2];
    fread(flags, 1, 2, f);

    int fname_length = ((flags[0] & 0x0F) << 9) + flags[1];
    entry->fname_length = fname_length;

    bool extended_flag = flags[0] & 0x40;
    if (version == 2 && extended_flag) {
        fprintf(stderr, "fatal: version 2 must have extended_flag = 0\n");
        exit(1);
    }

    int n_read = 62;

    entry->fname = (char *) malloc(fname_length + 1);
    fread(entry->fname, 1, fname_length + 1, f);
    n_read += fname_length + 1;

#if DEBUG
    printf("file:%s\n", entry->fname);
    printf("size:%u\n", entry->size);
    printf("sha1:%s\n", entry->sha1);
    printf("mtime:%ld\n", entry->mtime);
    printf("stage:%d\n", (flags[0] & 0xC0) >> 6);
    printf("\n");
#endif

    /* Pad such that n_read % 8 = 0 */
    int pad = 8 - (n_read % 8);
    if (pad != 8)
        fseek(f, pad, SEEK_CUR);

    return entry;
}

index_file_t *empty_index_file(void) {
    index_file_t *index = malloc(sizeof(*index));
    assert(index != NULL);
    index->entries = hash_table_init();
    return index;
}

index_file_t *read_index_file() {
    index_file_t *idx = empty_index_file();
    FILE *f = fopen(INDEX_PATH, "r");
    if (!f) return idx; // index may not exist if it has no entries

    // First 4 bytes are file signature
    char sig[4];
    fread(sig, 1, 4, f);
    if (memcmp(sig, DIRC_SIG, 4) != 0) {
        fprintf(stderr, "fatal: unexpected file signature\n");
        exit(1);
    }

    uint32_t version = read32(f);
    // Only support version 2
    if (version != 2) {
        fprintf(stderr, "fatal: invalid version; got %u but must be 2\n",
                version);
        exit(1);
    }

    size_t n_entries = read32(f);

    for (unsigned int i = 0; i < n_entries; i++) {
        index_entry_t *entry = read_index_entry(f, version);
        hash_table_add(idx->entries, entry->fname, entry);
    }

    fclose(f);
    return idx;
}

void write_index_entry(FILE *f, index_entry_t *entry) {
    if (entry->removed)  return;

    write32(f, 0);  // ctime seconds
    write32(f, 0);  // ctime nanoseconds
    write32(f, entry->mtime);  // mtime seconds
    write32(f, 0);  // mtime nanoseconds
    write32(f, 0);  // dev
    write32(f, 0);  // ino
    write32(f, entry->mode);
    write32(f, 0);  // uid
    write32(f, 0);  // gid
    write32(f, entry->size);  // file size

    uint8_t sha1[HASH_BYTES];
    hex_to_hash(entry->sha1, sha1);
    fwrite(sha1, 1, HASH_BYTES, f);

    // highest 4 bits use 0;
    // lower 12 bits use length of filename capped at 0xFFF
    uint16_t flags = (entry->fname_length < 0xFFF)
                      ? (uint16_t) entry->fname_length
                      : 0xFFF;
    write16(f, flags);

    fwrite(entry->fname, 1, entry->fname_length, f);
    fputc('\0', f);

    size_t bytes_written = 62 + entry->fname_length + 1;
    if (bytes_written % 8 > 0) {
        for (size_t i = 0; i < 8 - (bytes_written % 8); i++) {
            fputc('\0', f);
        }
    }
}

void write_index_file(const index_file_t *index) {
    FILE *f = fopen(INDEX_PATH, "w");
    assert(f);
    size_t nentries = hash_table_size(index->entries);

    /* Index Header */
    fwrite(DIRC_SIG, 1, 4, f);
    write32(f, SUPPORTED_VERSION);
    write32(f, nentries);

    /* Index entries */
    list_node_t *entries_it = key_set(index->entries);
    while (entries_it != NULL) {
        const char *fname = (const char *) node_value(entries_it);
        index_entry_t *entry = hash_table_get(index->entries, fname);
        write_index_entry(f, entry);
        entries_it = node_next(entries_it);
    }

    fclose(f);
}

void free_index_entry(index_entry_t *entry) {
    free(entry->fname);
    free(entry);
}
void free_index_file(index_file_t *idx) {
    free_hash_table(idx->entries, (free_func_t) free_index_entry);
    free(idx);
}

bool index_is_empty(index_file_t *index) {
    return hash_table_size(index->entries) == 0;
}

bool index_contains_file(index_file_t *index, const char *fname) {
    return hash_table_contains(index->entries, fname);
}

index_entry_t *index_create_entry(char *fname,
                                  struct stat *sb,
                                  object_hash_t sha1) {
    index_entry_t *entry = (index_entry_t *) malloc(sizeof(index_entry_t));
    assert(entry != NULL);
    entry->size = sb->st_size;
    strcpy(entry->sha1, sha1);
    strcpy(entry->fname, fname);
    entry->fname_length = strlen(fname);
    entry->mtime = sb->st_mtime;
    entry->mode = ((0b1000 << 12) |  // 4-bits for regular file
                   ((S_IRWXU | S_IRWXG | S_IRWXO) & sb->st_mode));
    entry->removed = false;
    return entry;
}

