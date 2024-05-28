#include "add.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "hash_table.h"
#include "object_io.h"
#include "index_io.h"
#include "util.h"

/* ASSUMPTION:
 *   git add [FILE1 FILE2 ...] always specifies FILEx as a relative path
 *   from the top of the git directory
 */
void add_files(const char **file_paths, size_t file_count)
{
  index_file_t *index = read_index_file();

  /* Get all files in working directory */
  const char *working_tree_path = getcwd(NULL, 0);  // malloc's big enough to hold cwd
#if DEBUG
  printf("add_files(): working_tree_path = %s\n", working_tree_path);
#endif
  hash_table_t *working_tree_files = hash_table_init();
  get_all_files(working_tree_path, working_tree_files, true);

  for (size_t i = 0; i < file_count; i++) {
    const char *added_file = file_paths[i];
#if DEBUG
    printf("add_files(): added_file = %s\n", added_file);
#endif
    if (hash_table_contains(working_tree_files,           // in working tree
                            added_file)) {
        struct stat sb;
        assert(stat(added_file, &sb) == 0);
        object_hash_t *sha1 = store_blob(added_file);  // add file to store, get its hash
        index_entry_t *new_entry = index_create_entry(added_file,
                                                      &sb,
                                                      *sha1);
        index_entry_t *old_entry = hash_table_add(index->entries,
                                                  added_file,
                                                  new_entry);
        if (old_entry != NULL) {
            free(old_entry);
        }
        free(sha1);
    } else if (index_contains_file(index, added_file)) {   // NOT in working tree, in index
        index_entry_t *entry = hash_table_get(index->entries, added_file);
        if (entry != NULL) {
            entry->removed = true;
        }
    }
  }

  write_index_file(index);
  free_hash_table(working_tree_files, (free_func_t) free_index_entry);
  free(working_tree_path);
}

