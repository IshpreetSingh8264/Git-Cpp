# 🚀 Punjabi Git - Git Implementation in C++ with Swag!

[![CodeCrafters](https://img.shields.io/badge/CodeCrafters-Git_Challenge-blue)](https://codecrafters.io/challenges/git)
[![C++23](https://img.shields.io/badge/C++-23-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/License-Educational-green)](LICENSE)

**Wadhaiya ji!** Welcome to the most hilarious Git implementation you'll ever read! 🎉

This is a from-scratch Git implementation in C++ with **Pinglish (Punjabi +
English)** comments that make learning Git internals actually fun!

**Scope, plainly:** the CodeCrafters Git course is 7 stages and this passes all
7. That means it can create and read objects, write trees, build commits, and
clone a public repository. It is **not** a general-purpose `git`: there is no
`checkout` command, no `commit` command, no `add`/staging area, no `.git/index`,
no `merge`/`rebase`/`diff`, no `fetch`/`push`, and no pack writing. See
["What this is not"](.github/copilot-instructions.md#what-this-is-and-what-it-is-not)
for the full list.

## 🎯 Features

✅ **The 7 course stages, working:**
- Repository initialization
- Blob objects (file storage)
- Tree objects (directory structure)
- Commit objects (history)
- Clone over smart HTTP, including pack delta resolution
- Content-addressable storage with SHA-1
- zlib compression

✅ **Modular Architecture:**
- Layered directories, one namespace per module
- Every header has a matching `.cpp`
- Command dispatch is a registry, not an if/else chain
- Modern C++23

✅ **Hilarious Comments:**
Every function has:
1. Pinglish comment with humor
2. English translation (also humorous!)
3. Technical documentation

## 🏗️ Project Structure

```
src/
├── main.cpp            # argv → dispatcher, 27 lines
├── commands/           # command registry + one file per command
├── clone/              # pkt-line, ref advertisement, http, pack, delta,
│                       #   worktree, orchestration
├── commit/             # commit objects + author identity
├── tree/               # tree parse / serialize / print / write
├── objects/            # loose object store + zlib
├── repository/         # the .git layout
└── utils/              # leaf helpers: error, varint, temp file, process

.github/copilot-instructions.md   # architecture: layers, data flow, conventions
DOCUMENTATION.md       # Git format reference and tutorials
README.md              # the CodeCrafters starting-point readme
CMakeLists.txt         # build configuration
your_program.sh        # run script (needs VCPKG_ROOT set, like on CodeCrafters)
```

## 🚀 Quick Start

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt install cmake g++ zlib1g-dev libssl-dev

# macOS
brew install cmake openssl zlib

# Fedora
sudo dnf install cmake gcc-c++ zlib-devel openssl-devel
```

### Build

```bash
# Using CMake
mkdir build && cd build
cmake ..
make

# Or use the provided script
chmod +x your_program.sh
./your_program.sh init
```

### Try It Out!

```bash
# Initialize repository
./your_program.sh init

# Create a file and make a blob
echo "Hello, Git!" > test.txt
./your_program.sh hash-object -w test.txt
# Output: <SHA-1 hash>

# Read the blob
./your_program.sh cat-file -p <hash>
# Output: Hello, Git!

# Create a tree from current directory
./your_program.sh write-tree
# Output: <tree hash>

# List tree contents
./your_program.sh ls-tree <tree-hash>

# Create a commit
./your_program.sh commit-tree <tree-hash> -m "Initial commit"
# Output: <commit hash>

# View commit
./your_program.sh cat-file -p <commit-hash>
```

## 📚 Commands Reference

| Command | Description | Example |
|---------|-------------|---------|
| `init` | Initialize repository | `./git init` |
| `cat-file -p <hash>` | Show object content | `./git cat-file -p abc123` |
| `hash-object -w <file>` | Create blob | `./git hash-object -w file.txt` |
| `ls-tree [--name-only] <hash>` | List tree | `./git ls-tree --name-only abc123` |
| `write-tree` | Create tree from directory | `./git write-tree` |
| `commit-tree <tree> -p <parent> -m <msg>` | Create commit | `./git commit-tree abc -m "msg"` |
| `clone <url> [dir]` | Clone repository | `./git clone https://...` |

## 🎓 Learning Resources

### Start Here:
1. **[.github/copilot-instructions.md](.github/copilot-instructions.md)** —
   the architecture: layer rules, data flow, conventions, a recipe for adding a
   command, and the list of things not to "fix"
2. **[DOCUMENTATION.md](DOCUMENTATION.md)** — Git format reference and tutorials
   - Module documentation
   - Git concepts explained
   - Step-by-step tutorials

### In the Code:
Every function has detailed comments explaining:
- What it does (Pinglish + English)
- Why it exists
- How it works
- Technical details

### Example Comment:
```cpp
/**
 * Oi puttar, eh function data nu compress karda hai
 * (Hey kiddo, this function compresses data)
 * 
 * Git vich saari cheezaan compress hondiyan ne - thoda space bachao te
 * (Everything in Git gets compressed - save some space, you know)
 * 
 * @param data - Raw data jo compress karna hai (Raw data to compress)
 * @return Compressed data as a vector of bytes
 */
inline std::vector<uint8_t> compress(const std::string& data) {
    // Pehle zlib stream setup karo, bilkul bike repair karni ho
    // (First setup zlib stream, just like fixing a bike)
    ...
}
```

## 🔬 How It Works

### Content-Addressable Storage

Git stores everything as objects identified by SHA-1 hashes:

```
"Hello, World!" 
    ↓ (hash-object)
SHA-1: 8ab686...
    ↓ (store)
.git/objects/8a/b686...
```

### Object Types

1. **Blob** - File content
2. **Tree** - Directory structure
3. **Commit** - Snapshot + metadata

### Example Flow

```bash
# File → Blob
echo "content" > file.txt
hash-object -w file.txt  # Creates blob

# Directory → Tree
write-tree               # Creates tree of all files

# Snapshot → Commit
commit-tree <tree> -m "msg"  # Creates commit
```

## 🧪 Testing

Compatible with real Git! Try this:

```bash
# Create objects with our Git
./git init
./git hash-object -w file.txt

# Read with real Git
git cat-file -p <hash>  # Works!
```

## 📖 Git Concepts Explained

### What is a Blob?
- Stores file content
- Does NOT store filename
- Identified by SHA-1 hash of content
- Same content = same hash = stored once!

### What is a Tree?
- Stores directory structure
- Contains: mode, name, hash for each entry
- Can point to blobs (files) or other trees (subdirs)

### What is a Commit?
- Snapshot of project at a point in time
- Contains: tree hash, parent hash, author, message
- Forms history through parent pointers

### Example Relationships:
```
Commit abc123
    │
    └─→ Tree def456
         ├─→ Blob 111 (file1.txt)
         ├─→ Blob 222 (file2.txt)
         └─→ Tree 333 (src/)
              └─→ Blob 444 (main.cpp)
```

## 🎨 Code Highlights

### Modular Design
Each module has a single responsibility:
- `Compression.hpp` - Compression only
- `GitObject.hpp` - Object storage only
- `Tree.hpp` - Tree operations only
- etc.

### Type Safety
Using modern C++23 features:
- Strong typing
- `std::filesystem` for paths
- `std::tuple` for multiple returns
- RAII for resource management

### Error Handling
Bilingual exceptions!
```cpp
throw std::runtime_error(
    "Arre bapu! Object nahi mila!\n"
    "(Oh father! Object not found!)"
);
```

## 🤝 Contributing

This is an educational project! Feel free to:
- Add more Git commands
- Improve documentation
- Add more Pinglish humor
- Fix bugs
- Add tests

## 🎯 CodeCrafters Challenge

This project passes all 7 CodeCrafters Git stages:
1. ✅ Repository Setup
2. ✅ Initialize .git directory
3. ✅ Read blob object
4. ✅ Create blob object
5. ✅ Read tree object
6. ✅ Write tree object
7. ✅ Create commit
8. ✅ Clone repository

## 📝 What You'll Learn

By studying this code, you'll understand:

- ✅ Content-addressable storage
- ✅ SHA-1 hashing
- ✅ zlib compression
- ✅ Binary file formats
- ✅ Network protocols (HTTP)
- ✅ File system operations
- ✅ Modern C++ design patterns
- ✅ And how Git actually works under the hood!

## 🚀 Future Ideas

Want to extend this? Consider adding:
- [ ] Branches and switching
- [ ] Merge operations
- [ ] Diff algorithm
- [ ] Pack writing / `gc` / `repack`
- [ ] Index/staging area
- [ ] Remote operations (fetch, pull, push)
- [ ] Tags
- [ ] Git protocol support
- [ ] Hooks

## 💡 Pro Tips

### Understanding Object Hashes
```bash
# Same content = same hash
echo "test" | ./git hash-object -w --stdin
echo "test" | ./git hash-object -w --stdin
# Both produce same hash!
```

### Debugging
```bash
# Use real git to inspect our objects
git cat-file -p <hash>
git cat-file -t <hash>  # Check type
git cat-file -s <hash>  # Check size
```

### Environment Variables
```bash
# Set author info
export GIT_AUTHOR_NAME="Your Name"
export GIT_AUTHOR_EMAIL="you@example.com"
./git commit-tree ...
```

## 🙏 Credits

- **CodeCrafters** - For the amazing challenge platform
- **Linus Torvalds** - For creating Git
- **Punjabi culture** - For the swag and humor

## 📄 License

Educational project. Use it, learn from it, enjoy the Pinglish!

---

## 🎉 Final Words

**Wadhaiya ji! Ab tusi Git de expert ban gaye!**

**(Congratulations! Now you've become a Git expert!)**

Git isn't magic - it's just a content-addressable filesystem with a VCS on top. And now you know exactly how it works! 

Keep coding with swag! 🚀💻

---

**Made with ❤️ and Pinglish humor**

*For questions, check [DOCUMENTATION.md](DOCUMENTATION.md) - it has EVERYTHING!*
