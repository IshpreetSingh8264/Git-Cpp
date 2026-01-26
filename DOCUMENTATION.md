# 🚀 Punjabi Git - Complete Git Implementation in C++

**Wadhaiya ji! (Congratulations!)** You've found the most swaggy Git implementation in C++!

This is a complete, modular Git implementation built for the CodeCrafters Git challenge. Every line is documented with hilarious Pinglish (Punjabi + English) comments that make learning Git internals fun!

---

## 📚 Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Modules Documentation](#modules-documentation)
4. [Git Concepts Explained](#git-concepts-explained)
5. [Building & Running](#building--running)
6. [Command Reference](#command-reference)
7. [How Git Actually Works](#how-git-actually-works)
8. [Testing](#testing)

---

## 🎯 Overview

### What is This?

This project implements core Git functionality from scratch in C++. It handles:

- ✅ **Repository Initialization** - Creating `.git` directory structure
- ✅ **Blob Objects** - Storing file contents
- ✅ **Tree Objects** - Storing directory structures
- ✅ **Commit Objects** - Creating snapshots with history
- ✅ **Clone** - Downloading remote repositories (basic HTTP support)
- ✅ **Content-Addressable Storage** - SHA-1 based object database
- ✅ **Compression** - zlib compression for efficient storage

### Why Pinglish Comments?

Because learning should be fun! Each function has:
1. **Pinglish comment** - Humorous explanation in Punjabi-English mix
2. **English translation** - Same thing in proper English (also humorous!)
3. **Technical documentation** - What the code actually does

---

## 🏗️ Architecture

### Modular Design

The codebase is organized into clean, focused modules:

```
src/
├── main.cpp           # Command routing and CLI interface
├── Compression.hpp    # zlib compression/decompression
├── GitObject.hpp      # Object database (blob, tree, commit)
├── Repository.hpp     # Repository initialization and management
├── Tree.hpp           # Tree object parsing and creation
├── Commit.hpp         # Commit object creation
└── Clone.hpp          # Remote repository cloning
```

### Design Principles

1. **Single Responsibility** - Each module has one clear purpose
2. **Header-Only** - All implementations in headers for simplicity
3. **Namespace Isolation** - Each module in its own namespace
4. **Error Handling** - Exceptions with bilingual error messages
5. **Type Safety** - Modern C++23 features

---

## 📖 Modules Documentation

### 1. Compression.hpp

**Purpose:** Handle zlib compression and decompression for Git objects

**Namespace:** `GitCompression`

**Key Functions:**

#### `compress(const std::string& data) -> std::vector<uint8_t>`
- **What:** Compresses raw data using zlib deflate
- **Why:** Git stores all objects compressed to save space
- **How:** 
  - Initializes zlib deflate stream
  - Processes data in 16KB chunks
  - Returns compressed byte vector
- **Pinglish:** "Dabao dabao, sab kucch compress karo!" (Press press, compress everything!)

#### `decompress(const std::vector<uint8_t>& compressed) -> std::string`
- **What:** Decompresses zlib compressed data
- **Why:** Need to read Git objects from disk
- **How:**
  - Initializes zlib inflate stream
  - Expands data in chunks
  - Returns original string
- **Pinglish:** "Phudko te expand karo! Balloon vaang!" (Puff up and expand! Like a balloon!)

**Technical Details:**
- Uses `Z_DEFAULT_COMPRESSION` level (6)
- Automatic header detection on inflate
- Chunk size: 16384 bytes (16KB)
- Full error handling with bilingual messages

---

### 2. GitObject.hpp

**Purpose:** Core object database operations - the heart of Git!

**Namespace:** `GitObject`

**Key Functions:**

#### `calculateSHA1(const std::string& data) -> std::string`
- **What:** Computes SHA-1 hash of data
- **Why:** Git uses SHA-1 for content-addressable storage
- **How:** 
  - Uses OpenSSL's SHA1 function
  - Converts binary hash to 40-char hex string
- **Returns:** "40-character hex string like `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`"
- **Pinglish:** "Har cheez di ek unique ID - jaise har punjabi da ek unique style!" 
  (Everything has a unique ID - like every Punjabi has a unique style!)

#### `readObject(const std::string& hash) -> std::string`
- **What:** Reads object from `.git/objects/` directory
- **Why:** Retrieve stored Git objects
- **How:**
  - Splits hash: first 2 chars = directory, rest = filename
  - Example: `e69de29...` → `.git/objects/e6/9de29...`
  - Reads binary file
  - Decompresses using `GitCompression::decompress()`
- **Format:** Returns full object with header: `"type size\0content"`
- **Pinglish:** "File kholo te padho - kitaab padhni vaang!" (Open and read file - like reading a book!)

#### `writeObject(const std::string& type, const std::string& content) -> std::string`
- **What:** Writes object to Git database
- **Why:** Store blobs, trees, commits
- **How:**
  1. Creates header: `"type size\0"`
  2. Combines with content
  3. Calculates SHA-1 hash
  4. Checks if object exists (de-duplication!)
  5. Creates directory: `.git/objects/XX/`
  6. Compresses content
  7. Writes binary file
- **Returns:** SHA-1 hash of created object
- **Pinglish:** "Compress kar ke .git/objects vich save karo!" (Compress and save in .git/objects!)

#### `createBlob(const std::string& content) -> std::string`
- **What:** Creates a blob object (Binary Large OBject)
- **Why:** Blobs store file contents in Git
- **How:** Simply calls `writeObject("blob", content)`
- **Note:** Blobs don't store filenames - just content!
- **Pinglish:** "Simple file content store karda hai, bass!" (Simply stores file content, that's it!)

#### `printBlob(const std::string& hash)`
- **What:** Prints blob content to stdout (like `git cat-file -p`)
- **How:**
  1. Reads object
  2. Splits header and content at null byte
  3. Validates it's a blob
  4. Prints content

#### `getObjectInfo(const std::string& hash) -> std::pair<std::string, size_t>`
- **What:** Returns object type and size without printing
- **Why:** Need to know what kind of object before processing
- **Returns:** `{"blob", 1234}` or `{"tree", 567}` or `{"commit", 890}`

---

### 3. Repository.hpp

**Purpose:** Git repository initialization and management

**Namespace:** `GitRepository`

**Key Functions:**

#### `init() -> bool`
- **What:** Initializes a new Git repository
- **Why:** Creates the `.git` directory structure
- **How:**
  1. Creates `.git/` directory
  2. Creates `.git/objects/` (object database)
  3. Creates `.git/refs/heads/` (branch references)
  4. Creates `.git/refs/tags/` (tag references)
  5. Creates `.git/HEAD` → `"ref: refs/heads/main\n"`
  6. Creates `.git/config` (repository configuration)
  7. Creates `.git/description` (repository description)
- **Pinglish:** "Ghar banana - pehle neev rakho, phir kamre banao!" 
  (Building a house - first lay foundation, then make rooms!)

**Directory Structure Created:**
```
.git/
├── HEAD                    # Points to current branch
├── config                  # Repository configuration
├── description             # Repository description
├── objects/                # Object database (blobs, trees, commits)
│   ├── XX/                 # First 2 chars of hash
│   │   └── YYYYYY...       # Rest of hash (38 chars)
└── refs/
    ├── heads/              # Branch references
    └── tags/               # Tag references
```

#### `isGitRepository() -> bool`
- **What:** Checks if current directory is a Git repository
- **How:** Simply checks if `.git/` exists and is a directory

#### `getGitDir() -> std::string`
- **What:** Returns path to `.git` directory
- **Throws:** If not in a Git repository

#### `readHEAD() -> std::string`
- **What:** Reads content of HEAD file
- **Returns:** Either `"ref: refs/heads/main"` or a commit hash (detached HEAD)

#### `getCurrentBranch() -> std::string`
- **What:** Gets current branch name
- **How:** Parses HEAD file, extracts branch from `"ref: refs/heads/XXX"`
- **Returns:** Branch name like `"main"` or `"develop"`

---

### 4. Tree.hpp

**Purpose:** Directory structure representation using tree objects

**Namespace:** `GitTree`

**Data Structures:**

#### `struct TreeEntry`
```cpp
struct TreeEntry {
    std::string mode;      // File permissions (100644, 100755, 040000)
    std::string name;      // File/directory name
    std::string hash;      // SHA-1 hash of object
};
```

**Modes Explained:**
- `040000` - Directory (tree object)
- `100644` - Regular file (non-executable blob)
- `100755` - Executable file (executable blob)
- `120000` - Symbolic link (not implemented yet)

**Key Functions:**

#### `parseTree(const std::string& content) -> std::vector<TreeEntry>`
- **What:** Parses tree object binary format
- **Why:** Tree objects store entries in binary format
- **Format:** Each entry is:
  ```
  mode<space>name<null>20-byte-binary-SHA1
  ```
- **Example:** `"100644 hello.txt\0" + binary_SHA1`
- **How:**
  1. Reads mode and name until null byte
  2. Splits on space to separate mode and name
  3. Reads next 20 bytes as binary SHA-1
  4. Converts binary to hex string
  5. Repeats for all entries
- **Pinglish:** "Ek ek karke entries parse karo - lassi vaang!" (Parse entries one by one - like churning lassi!)

#### `printTree(const std::string& hash, bool name_only = false)`
- **What:** Prints tree contents (like `git ls-tree`)
- **Modes:**
  - `name_only=true` → Just filenames (like `--name-only`)
  - `name_only=false` → Full details: `mode type hash\tname`
- **Output Example:**
  ```
  100644 blob e69de29bb2d1d6434b8b29ae775ad8c2e48c5391    hello.txt
  040000 tree 4b825dc642cb6eb9a060e54bf8d69288fbee4904    src
  ```

#### `writeTree(const std::string& path = ".") -> std::string`
- **What:** Creates tree object from directory
- **Why:** Captures directory snapshot for commit
- **How:**
  1. Scans directory with `std::filesystem`
  2. Skips `.git` folder (don't include Git metadata!)
  3. For each file:
     - Reads content
     - Creates blob object
     - Determines mode (executable or not)
  4. For each subdirectory:
     - Recursively calls `writeTree()`
     - Gets tree hash
  5. Sorts entries by name (Git's requirement!)
  6. Builds binary tree content
  7. Writes tree object
- **Returns:** SHA-1 hash of created tree
- **Pinglish:** "Recursively saari files te subdirectories process karo!" 
  (Recursively process all files and subdirectories!)

**Binary Format Details:**
```
Tree Object Format:
"tree <size>\0"
  + "mode1 name1\0" + 20-byte-binary-SHA1
  + "mode2 name2\0" + 20-byte-binary-SHA1
  + ...
```

---

### 5. Commit.hpp

**Purpose:** Create commit objects - snapshots with history!

**Namespace:** `GitCommit`

**Key Functions:**

#### `formatPersonInfo(...) -> std::string`
- **What:** Formats author/committer information
- **Parameters:**
  - `name` - Person's name
  - `email` - Email address
  - `timestamp` - Unix timestamp (optional, defaults to now)
- **Returns:** `"Name <email> timestamp timezone"`
- **Example:** `"Punjabi Coder <coder@punjab.dev> 1609459200 +0530"`
- **Timezone:** Automatically calculates local timezone offset

#### `createCommit(...) -> std::string`
- **What:** Creates a commit object
- **Parameters:**
  - `tree_hash` - Hash of tree object (required)
  - `parent_hash` - Hash of parent commit (empty for first commit)
  - `message` - Commit message
  - `author_name` - Who wrote the code
  - `author_email` - Author's email
  - `committer_name` - Who committed (optional, defaults to author)
  - `committer_email` - Committer's email (optional)
- **Returns:** SHA-1 hash of created commit

**Commit Object Format:**
```
commit <size>\0
tree <tree-hash>
parent <parent-hash>     # Optional - omitted for first commit
author Name <email> timestamp timezone
committer Name <email> timestamp timezone

Commit message here
Can be multiple lines
```

**Example Commit:**
```
tree 4b825dc642cb6eb9a060e54bf8d69288fbee4904
parent 1234567890abcdef1234567890abcdef12345678
author Punjabi Coder <coder@punjab.dev> 1609459200 +0530
committer Punjabi Coder <coder@punjab.dev> 1609459200 +0530

Initial commit - Pehli baar!
(First time!)
```

#### `readCommit(const std::string& hash) -> std::string`
- **What:** Reads and returns commit content
- **Validates:** Checks object is actually a commit

#### `printCommit(const std::string& hash)`
- **What:** Pretty-prints commit information

#### `getCommitTree(const std::string& hash) -> std::string`
- **What:** Extracts tree hash from commit
- **Why:** Need to checkout files from commit
- **How:** Parses commit content, finds `"tree "` line

#### `getAuthorFromEnv() -> std::pair<std::string, std::string>`
- **What:** Gets author info from environment
- **Checks:** 
  - `GIT_AUTHOR_NAME`
  - `GIT_AUTHOR_EMAIL`
- **Defaults:** `"Punjabi Coder"` and `"coder@punjab.dev"`

**Pinglish:** "Commit = snapshot of your work - ek moment capture karo!" 
(Commit = snapshot of your work - capture a moment!)

---

### 6. Clone.hpp

**Purpose:** Clone remote Git repositories

**Namespace:** `GitClone`

**Key Functions:**

#### `parseURL(const std::string& url) -> tuple<...>`
- **What:** Parses Git repository URL
- **Supports:** HTTP/HTTPS protocols
- **Format:** `https://github.com/user/repo.git`
- **Returns:** `(protocol, host, port, path)`
- **Examples:**
  - `https://github.com/user/repo.git` → `("https", "github.com", 443, "/user/repo.git")`
  - `http://example.com:8080/repo.git` → `("http", "example.com", 8080, "/repo.git")`

#### `sendHTTPRequest(...) -> std::string`
- **What:** Sends HTTP request to Git server
- **Protocol:** Git Smart HTTP Protocol
- **How:**
  1. Creates TCP socket
  2. Resolves hostname with `getaddrinfo()`
  3. Connects to server
  4. Sends HTTP GET request for `info/refs?service=git-upload-pack`
  5. Receives response
- **Returns:** Server response with references

**HTTP Request Format:**
```http
GET /repo.git/info/refs?service=git-upload-pack HTTP/1.1
Host: github.com
User-Agent: git/punjabi-git-cpp
Accept: */*
Connection: close
```

#### `clone(const std::string& url, std::string target_dir = "") -> bool`
- **What:** Clones repository to local directory
- **How:**
  1. Parses URL
  2. Creates target directory
  3. Initializes Git repository structure
  4. Fetches references from server
  5. Downloads pack file (simplified implementation)
  6. Extracts objects and files

**Note:** Full pack file parsing is very complex! This implementation provides:
- ✅ Repository structure initialization
- ✅ HTTP protocol communication
- ✅ Reference fetching
- ⚠️ Pack file parsing (basic - real Git uses delta compression)

**Pinglish:** "Remote repository nu local vich leke aa - saara code, history, sab kucch!" 
(Bring remote repository to local - all code, history, everything!)

---

## 🎓 Git Concepts Explained

### What is Git?

Git is a **content-addressable filesystem** with a VCS (Version Control System) built on top.

**Content-Addressable?** 
- Every piece of data has a unique SHA-1 hash
- Hash = address where content is stored
- Same content = same hash = stored only once!

### The Object Model

Git has 4 object types:

#### 1. **Blob** (Binary Large OBject)
- **Stores:** File content
- **Does NOT store:** Filename, permissions, directory
- **Example:** Content of `hello.txt`
- **Hash:** Based only on content
- **Deduplication:** Same content in multiple files = one blob!

#### 2. **Tree**
- **Stores:** Directory structure
- **Contains:** List of blobs and sub-trees
- **Each entry has:** Mode, name, hash
- **Represents:** Snapshot of directory at point in time

#### 3. **Commit**
- **Stores:** Metadata about a snapshot
- **Contains:**
  - Tree hash (what changed)
  - Parent commit(s) (history)
  - Author/Committer info
  - Timestamp
  - Commit message
- **Represents:** Point in project history

#### 4. **Tag** (not implemented yet)
- **Stores:** Named pointer to commit
- **Used for:** Releases, versions

### How Objects are Stored

1. **Content Creation:**
   ```
   Content: "Hello, World!"
   Header: "blob 13\0"
   Full: "blob 13\0Hello, World!"
   ```

2. **SHA-1 Hash:**
   ```
   SHA1("blob 13\0Hello, World!") = 8ab686eafeb1f44702738c8b0f24f2567c36da6d
   ```

3. **Compression:**
   ```
   Compressed = zlib_compress("blob 13\0Hello, World!")
   ```

4. **Storage:**
   ```
   Path: .git/objects/8a/b686eafeb1f44702738c8b0f24f2567c36da6d
   Content: <compressed binary data>
   ```

### The Commit Graph

```
Time →

(Initial)          (Second)          (Third)
Commit A    ←---   Commit B   ←---   Commit C
│                  │                  │
└─ Tree A          └─ Tree B          └─ Tree C
   │                  │                  │
   ├─ blob 1          ├─ blob 1          ├─ blob 1
   └─ blob 2          ├─ blob 2          ├─ blob 3 (changed!)
                      └─ blob 3          └─ blob 4 (new!)
```

Each commit points to:
- A tree (snapshot of all files)
- Its parent commit(s) (history)

---

## 🔧 Building & Running

### Prerequisites

```bash
# Required packages
sudo apt install cmake g++ zlib1g-dev libssl-dev

# Or on macOS
brew install cmake openssl zlib
```

### Build

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# Executable created: ./git
```

### Or Use Provided Script

```bash
./your_program.sh init
./your_program.sh cat-file -p <hash>
```

---

## 📋 Command Reference

### `init`
Initialize a new Git repository.

```bash
./git init
```

**Output:** `Initialized git directory`

**Creates:**
```
.git/
├── HEAD
├── config
├── description
├── objects/
└── refs/
    ├── heads/
    └── tags/
```

---

### `hash-object -w <file>`
Create a blob object from a file.

```bash
./git hash-object -w myfile.txt
```

**Output:** SHA-1 hash of created blob

**Example:**
```bash
echo "Hello, Git!" > test.txt
./git hash-object -w test.txt
# Output: 5ab2f8a4323abfdde98f0fa2b0ae6c46d51f4f67
```

---

### `cat-file -p <hash>`
Print content of a Git object.

```bash
./git cat-file -p 5ab2f8a4323abfdde98f0fa2b0ae6c46d51f4f67
```

**Output:** Object content (blob, tree, or commit)

**Works for:**
- Blobs → Prints file content
- Trees → Prints directory listing
- Commits → Prints commit information

---

### `ls-tree [--name-only] <tree-hash>`
List contents of a tree object.

```bash
# Full details
./git ls-tree 4b825dc642cb6eb9a060e54bf8d69288fbee4904

# Just names
./git ls-tree --name-only 4b825dc642cb6eb9a060e54bf8d69288fbee4904
```

**Output:**
```
100644 blob e69de29bb2d1d6434b8b29ae775ad8c2e48c5391    hello.txt
040000 tree 4b825dc642cb6eb9a060e54bf8d69288fbee4904    src
```

---

### `write-tree`
Create tree object from current directory.

```bash
./git write-tree
```

**Output:** SHA-1 hash of created tree

**What it does:**
1. Scans current directory recursively
2. Creates blobs for all files
3. Creates sub-trees for directories
4. Creates root tree object

---

### `commit-tree <tree-hash> -p <parent-hash> -m <message>`
Create a commit object.

```bash
./git commit-tree abc123... -p def456... -m "My commit message"
```

**Parameters:**
- `<tree-hash>` - Tree to commit (from `write-tree`)
- `-p <parent>` - Parent commit (omit for first commit)
- `-m <message>` - Commit message

**Output:** SHA-1 hash of created commit

**Example:**
```bash
# Create tree
TREE=$(./git write-tree)

# Create commit (first commit, no parent)
./git commit-tree $TREE -m "Initial commit"

# Create second commit with parent
COMMIT1="..."
TREE2=$(./git write-tree)
./git commit-tree $TREE2 -p $COMMIT1 -m "Second commit"
```

---

### `clone <url> [directory]`
Clone a remote repository.

```bash
./git clone https://github.com/user/repo.git
./git clone https://github.com/user/repo.git my-repo
```

**What it does:**
1. Creates target directory
2. Initializes Git repository
3. Fetches remote references
4. Downloads objects (basic implementation)

**Note:** Basic implementation - full pack file support is complex!

---

## 🔬 How Git Actually Works

### Step-by-Step: Creating a Commit

Let's walk through what happens when you commit:

```bash
# 1. Initialize repository
./git init

# 2. Create some files
echo "Hello" > file1.txt
echo "World" > file2.txt
mkdir src
echo "Code here" > src/main.cpp
```

**Step 1: Create Blobs**
```bash
# Git creates blob for each file
hash1 = hash-object -w file1.txt    # Blob for "Hello"
hash2 = hash-object -w file2.txt    # Blob for "World"  
hash3 = hash-object -w src/main.cpp # Blob for "Code here"
```

**Step 2: Create Trees**
```bash
# Tree for src/ directory
src_tree = tree {
  100644 blob hash3  src/main.cpp
}

# Tree for root directory
root_tree = tree {
  100644 blob hash1  file1.txt
  100644 blob hash2  file2.txt
  040000 tree src_tree  src
}
```

**Step 3: Create Commit**
```bash
commit = commit {
  tree: root_tree
  author: Punjabi Coder <coder@punjab.dev> 1609459200 +0530
  committer: Punjabi Coder <coder@punjab.dev> 1609459200 +0530
  
  Initial commit!
}
```

**Object Relationships:**
```
Commit abc123
    │
    └─→ Tree def456 (root)
         ├─→ Blob 111 (file1.txt)
         ├─→ Blob 222 (file2.txt)
         └─→ Tree 333 (src/)
              └─→ Blob 444 (main.cpp)
```

### Content-Addressable Magic

**Scenario:** Two files with same content

```bash
echo "Same content" > file_a.txt
echo "Same content" > file_b.txt

./git hash-object -w file_a.txt  # Hash: abc123...
./git hash-object -w file_b.txt  # Hash: abc123... (SAME!)
```

**Result:** Only ONE blob stored! Git automatically deduplicates.

**Storage:**
```
.git/objects/ab/c123...   ← Stored once
```

**Tree:**
```
100644 blob abc123  file_a.txt   ← Points to same blob
100644 blob abc123  file_b.txt   ← Points to same blob
```

### Why Compression?

**Without Compression:**
```
Content: "Hello, World!\n" (14 bytes)
Header:  "blob 14\0"       (8 bytes)
Total:   22 bytes
```

**With Compression:**
```
Compressed: ~15 bytes (zlib overhead for small files)
```

**For larger files:**
```
1MB file → ~300KB compressed (typical)
Savings: 70%!
```

---

## 🧪 Testing

### Manual Testing

```bash
# Test repository initialization
./git init
ls -la .git/

# Test blob creation
echo "Test content" > test.txt
HASH=$(./git hash-object -w test.txt)
echo $HASH

# Verify blob
./git cat-file -p $HASH

# Test tree creation
TREE=$(./git write-tree)
./git ls-tree $TREE

# Test commit creation
export GIT_AUTHOR_NAME="Tester"
export GIT_AUTHOR_EMAIL="test@example.com"
COMMIT=$(./git commit-tree $TREE -m "Test commit")
./git cat-file -p $COMMIT
```

### Integration with Real Git

Our Git is compatible with real Git!

```bash
# Create repo with our Git
./git init
echo "Hello" > file.txt
./git hash-object -w file.txt

# Read with real Git
git cat-file -p <hash>   # Works!

# Or vice versa
git init
git add file.txt
git write-tree
./git cat-file -p <tree-hash>  # Works!
```

---

## 🎉 What You've Learned

By reading this code, you now understand:

1. ✅ **How Git stores data** - Content-addressable object database
2. ✅ **SHA-1 hashing** - Unique IDs for content
3. ✅ **Compression** - zlib deflate/inflate
4. ✅ **Object types** - Blobs, trees, commits
5. ✅ **Directory snapshots** - Tree objects
6. ✅ **Commit history** - Parent pointers
7. ✅ **Network protocols** - HTTP Git protocol basics
8. ✅ **File system operations** - Creating directory structures
9. ✅ **Binary formats** - Parsing Git's binary tree format
10. ✅ **Modern C++** - C++23 features, namespaces, RAII

---

## 🚀 Future Enhancements

Want to contribute? Here are ideas:

- [ ] **Branches** - Creating and switching branches
- [ ] **Merge** - Combining branches
- [ ] **Diff** - Showing changes between commits
- [ ] **Pack files** - Full pack file parsing with delta compression
- [ ] **Index (staging area)** - The `.git/index` file
- [ ] **Remote tracking** - Fetch, pull, push
- [ ] **Tags** - Named commits
- [ ] **Git protocol** - Native Git protocol support
- [ ] **SSH** - SSH protocol for cloning
- [ ] **Refs** - Better reference handling
- [ ] **Garbage collection** - Cleanup unreferenced objects
- [ ] **Hooks** - Pre-commit, post-commit hooks

---

## 📝 License

This is an educational project for CodeCrafters. Use it, learn from it, laugh at the Pinglish comments!

---

## 🙏 Acknowledgments

- **CodeCrafters** - For the amazing challenge
- **Linus Torvalds** - For creating Git
- **Punjabi culture** - For the swag and humor

---

## 💡 Final Words

Remember:
> "Git is not as complicated as it seems. It's just a content-addressable filesystem with a VCS on top. Simple!" - Every Git tutorial ever

But seriously, Git internals are beautiful! Once you understand objects, trees, and commits, everything else makes sense.

**Wadhaiya ji! Ab tusi Git de expert ban gaye!**
**(Congratulations! Now you've become a Git expert!)**

Keep coding with swag! 🚀
