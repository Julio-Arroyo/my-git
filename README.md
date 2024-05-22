# my-git

## What is git?
Serves the purpose of a VCS (version control system), implemented as key-value store.

## git terminology
**objects**: blobs, trees, commits
**hash**: key for a git object. It's a SHA-1 hash computed as the hash
          of a header (consisting of the object type followed by object
          size) and the object contents.
**blobs**: correspond to files. Upon 'git add', blobs are stored under
           ".git/objects", get assigned a SHA-1 hash as a key, and are
           zlib compressed.
**trees**: associate SHA-1 keys of blobs with their filenames, and group
           together blobs.
**commit**: associates human-memorable names and metadata (such as
            commit messages) with trees (which are snapshots of the
            codebase).
**HEAD**: special pointer to current branch.
**detached HEAD state**: .git/HEAD refers to specific commit rather than
                         branch.
**branch**: pointer to commit
**index**: AKA staging area
**working tree**: repository that git tracks

## git subcommands to implement
### 1. git add
- Parts of .git modified
  - .git/index
  - .git/objects

- Computes a header for the file, hashes the header plus file contents,
  compresses the file contents, stores the deflated contents under
  .git/objects with the hash as filename.
- Plumbing commands
    - `git hash-object`
    - `git update-index`
        > --add: if file isn't in staging area (I think it writes it to store too)
        > --cacheinfo
        > creates/modifies the file ".git/index"
- "staging area": before writing out the index to the file
- delta compression doesn't happen upon adding: rather after doing 'git gc' or 'git push'

#### Examples
**Example 1**
```
repo/
  me.txt
```

If you do `git add me.txt`:
- me.txt is zlib-compressed, hashed, and stored as its hash under .git/objects.
- .git/index is also updated with the info of me.txt
and then you do `git commit -m "Add my file."`:
- a tree object containing me.txt is added to the store
- a commit object pointing to the tree above is added to the store

**Example 2**
```
repo/
  subdir/
    me.txt
```

If you do `git add subdir/`:
- nothing happens
If you do `git add subdir/me.txt`:
- subdir/me.txt is zlib-compressed, hashed, and stored as its hash under .git/objects.
- .git/index is also updated with the info of subdir/me.txt
- a tree object is not created for subdir. only the object for subdir/me.txt
and then you do `git commit -m "Add my file."`:
- a tree object containing me.txt is added to the store
- a commit object pointing to the tree above is added to the store

### 2. git status
#### Presence in HEAD ==> Presence in Index
If a file is in the HEAD, then it's on the index.

#### Examples
1. 
#### Untracked files
A file is **untracked** if and only if it exists in the working tree
but not the index
- Before adding: UNTRACKED
- After adding, before committing:

### 3. git commit
Parts of the database modified:
- .git/objects (creates a root tree object and a commit object pointing
  to it)
- .git/HEAD (if in detached HEAD state). Otherwise, .git/refs to update
  the ref that .git/HEAD points to.

### 4. git log
### 5. git checkout
Parts of the database modified:
- .git/HEAD
Checkout changes the index, basically resets it to the root tree of HEAD

### 6. git fetch
### 7. git push

