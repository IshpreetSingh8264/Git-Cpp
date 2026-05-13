# Git — C++

A Git implementation in C++23, built for the
[CodeCrafters "Build your own Git" challenge](https://codecrafters.io/challenges/git).

Loose objects hashed with SHA-1 and compressed with zlib, a full PACK v2/v3 reader with `OFS_DELTA` and `REF_DELTA`
resolution, and a smart-HTTP clone. Seven subcommands, no dependencies beyond OpenSSL and zlib.

> **Scope note.** This covers the seven stages the course asks for and then some. There is no index or staging area, no
> `branch`/`checkout`/`merge`, and no pack *writer*. See [Not implemented](#not-implemented).

## Contents

- [Quick start](#quick-start)
- [Commands](#commands)
- [Object model](#object-model)
- [Packfiles](#packfiles)
- [Clone](#clone)
- [Architecture](#architecture)
- [Tests](#tests)
- [Not implemented](#not-implemented)
- [Project layout](#project-layout)

## Quick start

Requires CMake 3.13+, a C++23 compiler, OpenSSL, and zlib development headers.

```bash
# Debian / Ubuntu
sudo apt install libssl-dev zlib1g-dev

cmake -S . -B build
cmake --build build
./build/git init
```

`vcpkg.json` declares no dependencies, so no toolchain file is needed for a local build. The `-DCMAKE_TOOLCHAIN_FILE`
line in `your_program.sh` is only there because the CodeCrafters image provides `VCPKG_ROOT`.

## Commands

The CLI is `git <subcommand> [args...]`. There are no flags, no `--help`, and no option parsing of any kind.

| Command | Form | Notes |
|---|---|---|
| `init` | `git init` | Takes no arguments. Creates `.git/`. Re-runnable. |
| `cat-file` | `git cat-file -p <hash>` | `-p` is required. Prints blobs, trees, and commits. |
| `hash-object` | `git hash-object -w <file>` | `-w` is required. Prints the new blob's hash. |
| `ls-tree` | `git ls-tree [--name-only] <hash>` | One tree, one hash. `--name-only` may appear anywhere. |
| `write-tree` | `git write-tree` | No arguments. Snapshots the working directory. |
| `commit-tree` | `git commit-tree <tree> [-p <parent>] -m <msg>` | One parent, one message. |
| `clone` | `git clone <url> [dir]` | Smart HTTP. `dir` defaults to the URL basename. |

Every command operates on `.` — there is no `-C`, no `--git-dir`, and no upward directory walk. Run it from the root of
the repository.

```bash
$ git init
Initialized empty Git repository in /home/you/demo/.git/
$ git hash-object -w README.md
8ab686eafeb1f44702738c8b0f24f2567c36da6d
$ git write-tree
4b825dc642cb6eb9a060e54bf8d69288fbee4904
$ git commit-tree 4b825dc642cb6eb9a060e54bf8d69288fbee4904 -m "first"
e5c1a2f0d4e3b1c9a8f7e6d5c4b3a29180f1e2d3
$ git cat-file -p e5c1a2f0d4e3b1c9a8f7e6d5c4b3a29180f1e2d3
tree 4b825dc642cb6eb9a060e54bf8d69288fbee4904
author Ishpreet Singh <ishpreetsingh8264@gmail.com> 1769387208 +0530
committer Ishpreet Singh <ishpreetsingh8264@gmail.com> 1769387208 +0530

first
```

## Object model

**SHA-1, over the full object with its header.** The hash input is `"<type> <size>\0<body>"`, so
`SHA1("blob 13\0Hello, World!")` is `8ab686eafeb1f44702738c8b0f24f2567c36da6d`. This is Git's format v1; there is no
SHA-256 support.

**Loose objects only.** Written to `.git/objects/<first 2 hex>/<remaining 38 hex>`, zlib-compressed. Writing is
idempotent: if the file already exists, the hash is returned and nothing is rewritten.

**Trees** serialise as `<mode> <name>\0<20 raw SHA-1 bytes>` per entry, sorted by name, exactly as `git ls-tree`
expects. Directories are written with mode `40000`.

**Commits** are `tree` / optional `parent` / `author` / `committer` / blank line / message. One parent maximum.

**Identity** resolves in this order, first hit wins:

1. `GIT_AUTHOR_NAME` / `GIT_AUTHOR_EMAIL`
2. `GIT_COMMITTER_NAME` / `GIT_COMMITTER_EMAIL` (if either author variable is empty)
3. `user.name` / `user.email` in `.git/config`
4. `A U Thor <author@example.com>`, with a warning on stderr

The fallback is Git's own, and it is deliberate — real Git errors here, but the course runs with an empty environment.

## Packfiles

`clone` is where packfile support matters, and the reader is the substantial part of this project.

`GitPack::PackReader` handles PACK versions 2 and 3 and every object type that appears in practice: `commit`, `tree`,
`blob`, `tag`, `OFS_DELTA`, and `REF_DELTA`.

- Object headers are 3 type bits plus a variable-length size, 7 bits per continuation byte, with an overflow guard.
- `OFS_DELTA` bases are located by a backwards offset varint; `REF_DELTA` bases by 20 binary SHA-1 bytes.
- The inflated size of every object is checked against the size the pack header declared. A mismatch is a hard
  corruption error, not a warning.
- Deltas resolve recursively — a `REF_DELTA` whose base is itself a delta result is fine — and results are memoised by
  both offset and hash so nothing is materialised twice.
- A failure on one object does not abandon the pack. The rest still lands, and the failure is recorded in `Stats`.

`GitDelta::apply` implements the standard format: a base size, a result size, then copy and insert instructions. A copy
of size zero means 64 KiB, per spec.

If a `REF_DELTA` base is not in the pack and not in the loose object store, it is recorded in `missingBases` and the
object is skipped. `clone` then issues a follow-up upload-pack request naming those hashes, up to five rounds.

**There is no pack writer, no `.idx` file, and no `gc`.**

## Clone

```
git clone <url>
  │
  ├─ parseUrl()                          scheme, host, port, path
  ├─ create_directories(target)
  ├─ createSkeleton(target)              .git/ without the "Initialized" banner
  ├─ fetchRefAdvertisement()             GET  /info/refs?service=git-upload-pack
  ├─ parseRefAdvertisement()             pkt-line, capabilities, default branch
  ├─ fetchPack()                         POST /git-upload-pack
  ├─ PackReader::read()                  resolve, write loose objects
  ├─ thin-pack follow-up                 up to 5 rounds for missing bases
  ├─ writeAllRefs() + writeHead()
  └─ checkoutHead()                      materialise the worktree
```

**Transport is a `curl` subprocess**, not a socket. `src/utils/process.cpp` does `fork()` + `execvp()` with no shell
involved, and `http_transport.cpp` hands `curl` a temporary file for the binary request body. Only `http` and `https`
are accepted.

**pkt-line** parsing reads a 4-hex-digit length that includes itself, treats `0000` as a flush, and `0001` as a
delimiter that ends the ref list.

**Default branch resolution** tries, in order: the `symref=HEAD:` capability target, `refs/heads/main`,
`refs/heads/master`, then the first `refs/heads/*`. It is never hard-coded to `main`, which is why the test suite can use
a repository whose only branch is `trunk`.

**Checkout** follows up to 8 `ref:` hops from `HEAD`, then walks the tree. Directories are created and recursed into,
mode `120000` entries become real symlinks, and `100755` gets the executable bits. A missing blob is **never** papered
over with an empty file — it is collected and the whole checkout fails with an error naming every missing object.

## Architecture

```
src/
  main.cpp                 27 lines: argv → dispatch
  commands/
    dispatcher.cpp         banner, usage, registry lookup, error handling
    registry.cpp           the seven-entry map
    *_command.cpp          one file per subcommand
  objects/
    object_store.cpp       hash, write, read, split
    compression.cpp        zlib deflate/inflate
  tree/tree_object.cpp     tree parse/serialise, writeTree, printTree
  commit/commit_object.cpp commit format, identity resolution
  repository/repository.cpp .git skeleton, HEAD, ref resolution
  clone/
    clone_operation.cpp    the orchestrator
    http_transport.cpp     curl wrappers, URL parsing
    pkt_line.cpp           ref advertisement framing
    ref_advertisement.cpp  refs, capabilities, default branch
    pack_reader.cpp        PACK v2/v3
    delta_applier.cpp      delta copy/insert
    worktree_writer.cpp    checkout
  utils/                   error type, process spawn
```

Layers do not depend upward. `commands/` may use anything below it; `objects/`, `tree/`, `commit/`, and `repository/`
know nothing about commands or the network; `clone/` composes them.

Most `src/*` files carry a bilingual Punjabi/English header comment explaining what the file is for. That is the house
style and is worth keeping.

## Tests

```bash
./tests/run.sh              # configure, build, both suites, size budget
./tests/run.sh pack         # synthetic pack reader only
./tests/run.sh clone        # clone over smart HTTP only

cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure
```

The clone suite needs `git`, `python3`, and `git-http-backend` on `PATH` (or in `/usr/lib/git-core/` or
`/usr/libexec/git-core/`). It stands up a local HTTP server backed by git's own CGI handler, so it exercises the real
wire protocol rather than a mock. It also clones the same repository with real `git` and asserts the two clones are
identical, file for file and object for object.

**44 assertions total**: 24 in the pack reader suite, 20 in the clone suite.

The pack suite hand-builds a 7-object pack byte by byte, covering a raw blob, an `OFS_DELTA` on it, a `REF_DELTA` on the
*result* of that delta (so resolution has to recurse), a delta whose base is missing, and a delta whose base exists only
in the loose object store.

`tests/check_size.py` fails if any test file passes 500 lines, so the suites cannot quietly grow into monoliths.

**Not covered**, and stated so in `tests/README.md`: HTTPS, dumb HTTP, `git://`, SSH, redirects, chunked transfer
encoding, any large repository, and the thin-pack follow-up fetch end to end — that last one is called out as the
highest-value gap.

The `test.sh` at the repository root is a manual scratch script, not a gate. It is not wired into `tests/run.sh`.

## Not implemented

- **No index or staging area.** `write-tree` snapshots the working directory, so untracked and tracked files are
  indistinguishable.
- **No `branch`, `checkout`, `switch`, `merge`, `rebase`, `reset`, `status`, `diff`, `log`.** Branches exist as ref
  files that `init` and `clone` write; nothing creates or moves them.
- **`commit-tree` does not update `HEAD`.** It writes the object and prints the hash, so a repository created with `init`
  points `HEAD` at a ref that does not exist yet.
- **No pack writing**, no `.idx`, no multi-pack-index, no SHA-256 repositories, no delta *creation*.
- **No annotated tag objects.** Tag type 4 can be read out of a pack, and `clone` writes `refs/tags/*` files, but
  nothing creates a tag.
- **Smart HTTP v0/v1 only.** No protocol v2, no auth, no `git://`, no SSH, no dumb HTTP.
- **`writeTree` skips symlinks.** It can read mode `120000` and checkout will create the symlink, but `writeTree` never
  emits the mode, so a symlink in the worktree silently disappears from a tree you write.
- **`hash-object --stdin` is not implemented.** Only `-w <file>`.
- **Tree entry sort is plain byte-wise name sort.** Git's real rule sorts a directory as though its name ended in `/`.
  This agrees for the ASCII names in the test fixtures, but it is not a guarantee of byte-for-byte parity.

## Licence

No licence file is present in this repository. Add one before redistributing.
