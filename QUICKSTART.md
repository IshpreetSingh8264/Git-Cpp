# 🚀 Quick start

## ✨ What this is

A from-scratch Git implementation in C++ that passes all **7** CodeCrafters Git
stages. It can create and read objects, write trees, build commits, and clone a
public repository. It is not a general-purpose `git` — there is no `checkout`
command, no `commit` command, no staging area, and no `merge`/`rebase`/`diff`.
The full scope is in
[`.github/copilot-instructions.md`](.github/copilot-instructions.md).

---

## 📦 Project Structure

```
codecrafters-git-cpp/
│
├── src/                          # Source code
│   ├── main.cpp                  # argv → dispatcher, 27 lines
│   ├── commands/                 # command registry + one file per command
│   ├── clone/                    # pkt-line, refs, http, pack, delta, worktree
│   ├── commit/                   # commit objects + author identity
│   ├── tree/                     # tree parse / serialize / print / write
│   ├── objects/                  # loose object store + zlib
│   ├── repository/               # the .git layout
│   └── utils/                    # leaf helpers (varint, temp file, process)
│
├── build/                        # Compiled binaries
│   └── git                       # Main executable
│
├── Documentation Files
│   ├── .github/copilot-instructions.md  # architecture, data flow, conventions
│   ├── DOCUMENTATION.md          # Technical guide
│   ├── PROJECT_README.md         # User guide
│   └── QUICKSTART.md             # This file
│
├── Build Configuration
│   ├── CMakeLists.txt            # CMake build config
│   ├── vcpkg.json                # Package dependencies
│   └── vcpkg-configuration.json  # vcpkg config
│
└── Scripts
    ├── your_program.sh           # CodeCrafters run script
    └── test.sh                   # Automated test suite
```

---

## 🚀 Quick Start

### 1. Build the Project

```bash
# From project root
cmake -S . -B build
make -C build -j$(nproc)

# Executable created: ./build/git ✅
```

### 2. Test It Out!

```bash
# Go to a test directory
cd /tmp && mkdir my-repo && cd my-repo

# Initialize repository
"/path/to/codecrafters-git-cpp/build/git" init
# Output: Initialized git directory

# Create a file
echo "Hello, Git!" > hello.txt

# Create blob object
"/path/to/codecrafters-git-cpp/build/git" hash-object -w hello.txt
# Output: <40-char SHA-1 hash>

# Read the blob
"/path/to/codecrafters-git-cpp/build/git" cat-file -p <hash>
# Output: Hello, Git!

# Create tree from current directory
"/path/to/codecrafters-git-cpp/build/git" write-tree
# Output: <tree hash>

# Create commit
export GIT_AUTHOR_NAME="Your Name"
export GIT_AUTHOR_EMAIL="you@example.com"
"/path/to/codecrafters-git-cpp/build/git" commit-tree <tree-hash> -m "First commit!"
# Output: <commit hash>
```

---

## 🎯 All Commands Available

| Command | What It Does | Example |
|---------|--------------|---------|
| `init` | Create new Git repository | `./git init` |
| `hash-object -w <file>` | Store file as blob | `./git hash-object -w file.txt` |
| `cat-file -p <hash>` | Display object content | `./git cat-file -p abc123` |
| `ls-tree <hash>` | List tree entries | `./git ls-tree abc123` |
| `ls-tree --name-only <hash>` | List only names | `./git ls-tree --name-only abc123` |
| `write-tree` | Create tree from directory | `./git write-tree` |
| `commit-tree <tree> -m <msg>` | Create commit | `./git commit-tree abc123 -m "msg"` |
| `commit-tree <tree> -p <parent> -m <msg>` | Create commit with parent | `./git commit-tree abc -p def -m "msg"` |
| `clone <url> [dir]` | Clone remote repository | `./git clone https://...` |

---

## 📚 Documentation Files

### 1. **DOCUMENTATION.md** (Git format reference)
   - Architecture overview
   - Every module explained in detail
   - Git concepts from scratch
   - How Git actually works internally
   - Step-by-step tutorials
   - Testing guide
   - Future enhancements

### 2. **PROJECT_README.md** (User Guide)
   - Quick start guide
   - Command reference with examples
   - Learning resources
   - Pro tips
   - Contributing guidelines

### 3. **Inline Code Comments**
   - Every function documented
   - Pinglish + English explanations
   - Technical details
   - Examples

---

## 🎨 Special Features

### 1. Hilarious Pinglish Comments
Every function has bilingual humor:
```cpp
// Oi puttar, eh function data nu compress karda hai
// (Hey kiddo, this function compresses data)

// Dabao dabao, sab kucch compress karo!
// (Press press, compress everything!)

// Bilkul bike repair karni ho
// (Just like fixing a bike)
```

### 2. Modular Architecture
- Each directory is one layer, and calls only downward
- One namespace per module
- Every header that declares behaviour has a matching `.cpp`
- Command dispatch is a registry, not an if/else chain

### 3. Modern C++23
- `std::filesystem` for paths
- Structured bindings
- `std::tuple` for returns
- Type-safe design

### 4. Loud Error Handling
Errors are thrown as `GitError::GitError` and printed by the dispatcher, which
sets a non-zero exit code. Nothing is faked to make a run look successful:
```cpp
throw GitError::GitError("Object nahi mila: " + hash);
```

---

## ✅ Verification

The 7 course stages are the gate, and they pass:
```sh
codecrafters test   # 7/7
```

```bash
✅ Repository initialization
✅ .git directory structure creation
✅ Blob object creation
✅ Blob object reading
✅ Tree object creation
✅ Tree object parsing
✅ Tree listing (full and name-only)
✅ Commit creation (with and without parent)
✅ Commit reading
✅ SHA-1 hashing
✅ zlib compression/decompression
✅ Content-addressable storage
✅ Clone (basic HTTP support)
```

---

## 🎓 What You've Learned

By studying this codebase, you now understand:

1. **Git Internals**
   - Content-addressable storage
   - Object database (blobs, trees, commits)
   - SHA-1 based addressing
   - Compression techniques

2. **Data Structures**
   - Binary tree format
   - Commit graph
   - Object relationships

3. **C++ Programming**
   - Modern C++23 features
   - Modular design patterns
   - Splitting headers from translation units
   - Namespace organization

4. **System Programming**
   - File I/O operations
   - Binary data handling
   - Network sockets
   - HTTP protocol basics

5. **Software Engineering**
   - Clean architecture
   - Error handling
   - Documentation practices
   - Testing strategies

---

## 🏆 CodeCrafters Challenge

All 7 stages of the course pass:
1. ✅ Repository Setup
2. ✅ Initialize .git directory
3. ✅ Read blob object
4. ✅ Create blob object
5. ✅ Read tree object
6. ✅ Write tree object
7. ✅ Create commit
8. ✅ Clone repository

---

## 🔧 Development Commands

```bash
# Build
cmake -S . -B build && make -C build

# Clean build
rm -rf build && cmake -S . -B build && make -C build

# Run
./build/git <command>

# Test in isolated environment
cd /tmp && mkdir test-repo && cd test-repo
/path/to/build/git init

# Check compatibility with real Git
git cat-file -p <hash>  # Read objects created by our Git
```

---

## 💡 Pro Tips

### Tip 1: Environment Variables
Set author info before creating commits:
```bash
export GIT_AUTHOR_NAME="Your Name"
export GIT_AUTHOR_EMAIL="your@email.com"
```

### Tip 2: Debugging
Use real Git to inspect objects created by our implementation:
```bash
# Create object with our Git
./git hash-object -w file.txt

# Inspect with real Git
git cat-file -p <hash>
git cat-file -t <hash>  # Check type
git cat-file -s <hash>  # Check size
```

### Tip 3: Understanding Hashes
Same content = same hash:
```bash
echo "test" > file1.txt
echo "test" > file2.txt
./git hash-object -w file1.txt  # Hash: abc123...
./git hash-object -w file2.txt  # Hash: abc123... (SAME!)
```

### Tip 4: Object Storage
Objects stored in: `.git/objects/XX/YYYYYY...`
- First 2 chars = directory name
- Remaining 38 chars = file name

---

## 🎯 Example Workflow

Complete example from init to commit:

```bash
# 1. Initialize
./git init

# 2. Create files
echo "Hello" > file1.txt
echo "World" > file2.txt
mkdir src
echo "int main() {}" > src/main.cpp

# 3. Create tree
TREE=$(./git write-tree)
echo "Tree: $TREE"

# 4. View tree
./git ls-tree $TREE

# 5. Create commit
export GIT_AUTHOR_NAME="Developer"
export GIT_AUTHOR_EMAIL="dev@example.com"
COMMIT=$(./git commit-tree $TREE -m "Initial commit")
echo "Commit: $COMMIT"

# 6. View commit
./git cat-file -p $COMMIT

# 7. Make changes and create second commit
echo "Updated" > file1.txt
TREE2=$(./git write-tree)
COMMIT2=$(./git commit-tree $TREE2 -p $COMMIT -m "Second commit")

# 8. View second commit (has parent!)
./git cat-file -p $COMMIT2
```

---

## 📖 Read More

- **DOCUMENTATION.md** - Start here for deep dive
- **PROJECT_README.md** - User-friendly guide
- **Source code** - Every function is documented!

---

## 🎉 Final Words

**Wadhaiya ji! Ab tusi Git de expert ban gaye!**
**(Congratulations! Now you've become a Git expert!)**

You now have:
- ✅ A working Git implementation
- ✅ Deep understanding of Git internals
- ✅ Modern C++ skills
- ✅ A portfolio project with personality!

### What Makes This Special?

1. **Functional** - It actually works!
2. **Educational** - Learn while laughing
3. **Modular** - Clean, extensible code
4. **Documented** - Everything explained
5. **Fun** - Pinglish humor throughout!

---

## 🚀 Next Steps

### Extend the Project
- Add branches and merging
- Implement diff algorithm
- Add staging area (index)
- Full pack file support
- Remote operations (push, pull, fetch)

### Share Your Knowledge
- Blog about Git internals
- Create tutorial videos
- Contribute to open source
- Teach others!

### Keep Learning
- Study real Git source code
- Explore other VCS systems
- Build more tools!

---

**Made with ❤️, C++23, and Pinglish humor!**

**Keep coding with swag! 🚀💻**

---

## 📞 Quick Reference Card

```
COMMANDS:
  init                    → Create repo
  hash-object -w <file>   → Store blob
  cat-file -p <hash>      → Show object
  ls-tree <hash>          → List tree
  write-tree              → Create tree
  commit-tree <tree> -m   → Create commit
  clone <url>             → Clone repo

DIRECTORIES:
  .git/objects/           → Object database
  .git/refs/heads/        → Branches
  .git/HEAD               → Current branch

OBJECT TYPES:
  blob   → File content
  tree   → Directory structure
  commit → Snapshot + metadata

BUILD:
  cmake -S . -B build && make -C build

RUN:
  ./build/git <command>
```

---

**Enjoy your Git implementation!** 🎊
