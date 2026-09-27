# Punjabi Git — architecture

C++23 implementation for the CodeCrafters
["Build Your Own Git"](https://codecrafters.io/challenges/git) challenge, written
with bilingual Punjabi/English comments. `git` here is a *from-scratch* reader
of Git's on-disk formats and wire protocols; it links nothing from `libgit2`.

## What this is, and what it is not

The course is **7 stages** and all 7 pass. That is the whole course.

| Implemented | Deliberately not implemented |
|---|---|
| `init`, `hash-object -w`, `cat-file -p`, `ls-tree`, `write-tree`, `commit-tree`, `clone` | `checkout` as a command, `commit` as a command, `rebase`, branch switching, `git diff` |
| Loose object store: read, write, SHA-1, zlib | `.git/index` (the staging area) and `add` |
| Tree parse/serialize/write, executable bit, symlinks | `multi-pack-index`, pack writing (`gc`, `repack`) |
| Clone over smart HTTP: ref advertisement, `want`/`have` negotiation, pack download, **OFS_DELTA and REF_DELTA resolution**, default-branch detection, working tree checkout | `fetch`, `pull`, `push`, SSH transport, the native git:// protocol, shallow clone, partial clone (filter) |

Delta resolution is *not* a course stage, but a clone that skips deltas writes a
corrupt working tree, so it is treated as part of "clone works" rather than as a
new feature. The same goes for the loud failure on a missing object: quietly
writing an empty file is worse than failing.

A clone is a faithful copy of the **default branch**: objects, all advertised
refs, and the working tree. It does not track the remote, and the object store
is loose-only.

## Layout — layered by role

```
src/
  main.cpp            27 lines. argv → dispatcher. Nothing else.
  commands/           the top layer: registry + one file per command
  clone/              the wire and the working tree
  commit/             commit objects + author identity
  tree/               tree parse / serialize / print / write
  objects/            loose object store + zlib
  repository/         the .git layout, and only that
  utils/              leaf helpers, no knowledge of Git
```

Dependencies point **downward only**:

```
commands → clone → {commit, tree, objects, repository} → utils
```

`utils/` knows nothing about Git (`varint`, `mkstemp`, `fork+execvp`).
`repository/` knows only paths. `objects/` knows only how a loose object is
laid out. Nothing calls back up.

Each module owns a namespace matching its directory: `GitClone`, `GitCommit`,
`GitTree`, `GitObject`, `GitRepository`, `GitCompression`, `GitCommands`,
`GitPack`, `GitDelta`, `GitHttp`, `GitPkt`, `GitWorktree`, `GitUtil`,
`GitError`.

## Module map

| File | Lines | Owns |
|---|---|---|
| `utils/error.hpp` | 29 | `GitError::GitError`. Contract only, so no `.cpp` |
| `utils/varint.{hpp,cpp}` | 66 | Git's 7-bit varint, shared by pack and delta parsing |
| `utils/temp_file.{hpp,cpp}` | 167 | `mkstemp` file that unlinks itself in the destructor |
| `utils/process.{hpp,cpp}` | 87 | `fork` + `execvp`, no shell |
| `repository/repository.{hpp,cpp}` | 226 | `.git` layout, `HEAD`, ref resolution, `init` |
| `objects/compression.{hpp,cpp}` | 127 | zlib deflate/inflate |
| `objects/object_store.{hpp,cpp}` | 242 | loose object read/write, SHA-1, `"<type> <size>\0<body>"` |
| `tree/tree_object.{hpp,cpp}` | 256 | `parseTree`, `serializeTree`, `ls-tree`, `write-tree` |
| `commit/identity.{hpp,cpp}` | 218 | author/committer lookup and `name <email> ts +ZZZZ` |
| `commit/commit_object.{hpp,cpp}` | 129 | commit build/read/print, `tree` extraction |
| `clone/pkt_line.{hpp,cpp}` | 114 | pkt-line framing |
| `clone/ref_advertisement.{hpp,cpp}` | 229 | refs, capabilities, `symref=HEAD:`, default branch |
| `clone/http_transport.{hpp,cpp}` | 207 | URL parsing, `info/refs` GET, `git-upload-pack` POST |
| `clone/delta_applier.{hpp,cpp}` | 137 | delta copy/insert instructions |
| `clone/pack_reader.{hpp,cpp}` | 409 | pack walk, OFS_DELTA/REF_DELTA resolution, stats |
| `clone/worktree_writer.{hpp,cpp}` | 195 | commit → working tree, missing-object detection |
| `clone/clone_operation.{hpp,cpp}` | 237 | the orchestration and the diagnostics |
| `commands/handlers.hpp` | 58 | `CommandContext`, the `Handler` alias, the `CommandRegistry` alias |
| `commands/registry.{hpp,cpp}` | 40 | the command map |
| `commands/dispatcher.{hpp,cpp}` | 94 | banner, usage, error catching, exit codes |
| `commands/*_command.cpp` | 7 files | one command each |

## Data flow

**Write path** (`write-tree` then `commit-tree`):

```
argv → dispatcher → registry["write-tree"]
  → GitTree::writeTree      walk the directory, recurse
  → GitObject::createBlob   zlib("blob <n>\0" + bytes) → SHA-1 → .git/objects/ab/cdef…
  → GitObject::writeObject  the same, for trees
  → stdout: the tree hash

argv → dispatcher → registry["commit-tree"]
  → GitCommit::identityFromEnv   GIT_AUTHOR_* → GIT_COMMITTER_* → .git/config
  → GitCommit::createCommit      assemble the commit text
  → GitObject::writeObject
  → stdout: the commit hash
```

**Read path** (`cat-file -p`):

```
argv → dispatcher → registry["cat-file"] → GitObject::getObjectInfo
  → GitObject::readObject   .git/objects/ab/cdef… → zlib inflate → split on the NUL
  → GitObject::printBlob | GitTree::printTree | GitCommit::printCommit
  → stdout
```

**Clone path:**

```
GitClone::clone
 1 GitHttp::parseUrl                    https://github.com/owner/repo → host, port, path
 2 GitHttp::fetchRefAdvertisement       GET  /info/refs?service=git-upload-pack
 3 GitPkt::readLines                    framing
 4 RefAdvertisement                     refs, capabilities, symref=HEAD:refs/heads/<branch>
 5 GitHttp::fetchPack                   POST /git-upload-pack with every want
 6 GitPack::PackReader::read
     walk:    header varint → type + size
              OFS_DELTA → base is a negative-offset varint
              REF_DELTA → base is a 20-byte hash
              inflate exactly `size` bytes
     resolve: OFS_DELTA → base earlier in this pack (memoised)
              REF_DELTA → base in this pack, else the object store
              apply the delta, then GitObject::writeObject
     any base that is nowhere → collect the hash, do not write the object
 7 fetchPack again for the collected hashes, with `have` lines, up to 5 rounds
 8 writeAllRefs + writeHead              every refs/* ref, HEAD to the default branch
 9 GitWorktree::checkoutHead
     resolveHeadCommit → getCommitTree → recurse trees → write blobs
     a missing blob is an error, never an empty file
10 stdout: "Clone complete!"
```

## Conventions

- **Every header that declares behaviour has a `.cpp`.** Headers are contract
  only, and the one exception is `utils/error.hpp`, which declares a class and
  nothing else, so there is nothing to link.
- **Every layer returns values or throws `GitError::GitError`.** No `exit()`
  outside `commands/` (only `clone_command.cpp` calls it, so `clone` can set a
  failure exit code), no silent `catch` that swallows a decode error.
- **stdout is the product.** Command output goes to stdout; progress,
  warnings and errors go to stderr. The dispatcher turns exceptions into
  `Error: <what>` on stderr and `EXIT_FAILURE`.
- **Bilingual comments are the house style.** Every function gets a Punjabi
  line, an English line, and `@param`/`@return` where it earns them. Keep it
  light; do not write a comment that only restates the code.
- **Never guess a size.** An object's size comes from the pack header, not
  from what `inflate` happened to return. A mismatch is corruption and is
  reported.
- **No `system()`.** Spawn with `GitUtil::runProcess` so nothing reaches a
  shell, and put bytes in a `GitUtil::TempFile` so they are cleaned up on every
  exit path.
- One namespace per module, matching the directory name.

## How to add a command

1. Write the handler in `src/commands/<name>_command.cpp`:

   ```cpp
   #include "commands/handlers.hpp"

   namespace GitCommands {

   void myCommand(const CommandContext& context) {
       std::string hash = context.argument(0);          // args after the command
       if (context.argumentCount() != 1) {              // 0 = the command took nothing
           std::cerr << "Usage: my-command <hash>\n";
           throw GitError::GitError("my-command lai ek hash chahida hai");
       }
       std::cout << doTheThing(hash) << "\n";
   }

   } // namespace GitCommands
   ```

2. Declare it in `src/commands/handlers.hpp` next to the other seven.
3. Add one line to `src/commands/registry.cpp`:

   ```cpp
   {"my-command", myCommand},
   ```

There is no `switch` and no `if` chain to edit, and no edit to `main.cpp`.
Argument parsing, error reporting and the exit code all come from the layer
above, so the handler only does its own job.

## How to change the pack format handling

`clone/pack_reader.cpp` is the only file that knows the pack layout. Its
`walk()` records offsets and inflated payloads; `materialize()` turns a delta
into a real object. Results are memoised in `resolvedOffset_` (by offset) and
`byHash_` (by hash), so a base is applied once. `clone_operation.cpp` only sees
`GitPack::Stats`, and reacts to `stats.missingBases` by asking the server for
those hashes with `have` lines — that is the thin-pack path.

## Tests

`codecrafters test` builds and runs all 7 stages; it is the gate for any change.
Locally, from a clean checkout:

```sh
./tests/run.sh
```

That builds and runs 45 assertions with no network. `ctest --test-dir build`
works too. **Read `tests/README.md` first** — it documents what these suites
deliberately do not cover, including the one gap that matters most (the
thin-pack follow-up fetch is not exercised end to end).

| Suite | Covers |
|---|---|
| `tests/unit/` | OFS_DELTA, REF_DELTA on an in-pack base, REF_DELTA on a store-only base, and an unresolvable base, against a synthetic pack with a known manifest |
| `tests/integration/` | a real clone over smart HTTP of a repo whose `HEAD` is `refs/heads/trunk` — neither `main` nor `master`, which the course fixture never exercises |

Do not run `init` inside this repository — it writes `.git` in the working
directory. `test.sh` in the repo root is an interactive demo, not a gate: it
exits 0 even when a test fails.

## Do not "improve" these

- **Do not add `checkout`, `commit`, `rebase`, `add`, the index, or
  `multi-pack-index`.** They are not on the course, and the course is finished.
  If a future brief asks for them, that is a scope change, not a bug fix.
- **Do not reintroduce a "skip the delta object" path.** It is the bug that
  made a clone quietly lose blobs.
- **Do not make a missing object non-fatal.** An empty file is indistinguishable
  from a legitimately empty one, which is exactly why the clone stage used to
  pass while writing files that do not exist upstream.
- **Do not fabricate an author identity.** If the environment and `.git/config`
  are both empty, the fallback is git's own `A U Thor <author@example.com>` and
  it prints a warning. The commit-tree stage runs with an empty environment, so
  failing there would fail a passing stage.
- **Do not reintroduce `system()`.** A URL is user input.
