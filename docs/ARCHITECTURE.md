# Git — Architecture

How the implementation is put together. The [README](../README.md) covers what it does; this covers the shape of the
code and the decisions worth knowing about.

> This file replaces `DOCUMENTATION.md`, `PROJECT_README.md`, and `QUICKSTART.md`. The latter two were roughly 80%
> identical to each other, and `DOCUMENTATION.md` described a pre-`curl`, pre-pack-reader version of the codebase,
> including a `sendHTTPRequest` function that opened a TCP socket — this project has never done that.

## Contents

- [Layering](#layering)
- [Adding a subcommand](#adding-a-subcommand)
- [The object store](#the-object-store)
- [Trees](#trees)
- [Commits and identity](#commits-and-identity)
- [The repository skeleton](#the-repository-skeleton)
- [Clone internals](#clone-internals)
- [The pack reader](#the-pack-reader)
- [Worktree checkout](#worktree-checkout)
- [Error handling](#error-handling)
- [Conventions](#conventions)

## Layering

```
commands/          the CLI: dispatch, registry, one file per subcommand
    │
    ├── objects/   hash, loose-object read/write, zlib
    ├── tree/      tree parse/serialise, writeTree, printTree
    ├── commit/    commit format, identity resolution
    ├── repository/  .git skeleton, HEAD, ref resolution
    └── clone/     the orchestrator plus transport, pkt-line, pack, checkout
            │
            └── utils/   GitError, fork+exec
```

Nothing below `commands/` knows that commands exist, and nothing in `objects/`, `tree/`, `commit/`, or `repository/`
knows about the network. `clone/` is the only module that composes the rest.

`src/main.cpp` is 27 lines: it copies `argv` into a vector and calls `GitCommands::dispatch`. All of it is argument
forwarding.

`CMakeLists.txt` builds everything except `main()` into a static `gitcore` library, so the unit tests link the same
object code the CLI does rather than a copy.

## Adding a subcommand

1. Write `src/commands/<name>_command.cpp` with a function matching `Handler`.
2. Declare it in `src/commands/handlers.hpp`.
3. Add one entry to the map in `src/commands/registry.cpp`.
4. Add a line to `printUsage()` in `dispatcher.cpp`.

`CommandRegistry` is a `std::map<std::string, Handler>`; there is no `switch` and no if-chain. `Handler` is
`void(CommandContext&)`, and `CommandContext::argument(i)` indexes past the subcommand, so `argument(0)` is the first
argument after it.

`dispatcher.cpp` sets `unitbuf` on both streams, prints a two-line banner to **stderr** on every invocation, checks the
argument count, looks the subcommand up, invokes the handler inside a `try`, and turns any `std::exception` into
`Error: <what>` on stderr plus `EXIT_FAILURE`.

## The object store

`GitObject::writeObject(root, type, body)`:

1. build `"<type> <size>\0" + body`
2. SHA-1 it (OpenSSL's one-shot `SHA1()`)
3. if the object file already exists, return the hash immediately
4. create the two-character fan-out directory
5. zlib-compress and write

`readObject` is the inverse and returns the *whole* thing, header included; `splitObject` cuts at the first `\0` and
then at the first space.

`objectPath` requires a 40-character hash and throws otherwise, so abbreviated hashes are not accepted anywhere.

`GitCompression` is a pair of free functions in `src/objects/compression.cpp` using `deflateInit` / `inflateInit` at
`Z_DEFAULT_COMPRESSION` with a 16 KiB scratch buffer. `inflateInit` rather than `inflateInit2`, so it auto-detects zlib
versus gzip framing.

## Trees

The on-disk entry format is `<mode> <name>\0<20 raw SHA-1 bytes>`. `parseTree` and `serializeTree` are exact inverses.

`TreeEntry::isDirectory()` accepts both `40000` and `040000`, which is deliberately more tolerant than Git on the read
side. `writeTree` emits `40000` for directories and `100755` or `100644` for files depending on the owner execute bit.

Two things it does *not* do, both documented in the README:

- It never emits `120000`, so symlinks are skipped rather than recorded. The reader and the checkout path both understand
  `120000`; only the writer ignores it.
- It sorts by plain byte-wise name. Git sorts a directory as though its name ended in `/`. The two agree for the ASCII
  names in the fixtures but are not equivalent in general.

`printTree` writes `<mode> <type> <hash>\t<name>`, matching `git ls-tree`.

## Commits and identity

`GitCommit::createCommit(root, author, committer, message, treeHash, parentHash)` writes:

```
tree <hash>\n
[parent <hash>\n]
author <name> <<email>> <ts> <+ZZZZ>\n
committer <name> <<email>> <ts> <+ZZZZ>\n
\n<message>\n
```

`printCommit` prepends a `commit <hash>` line before the raw body, which is what `git cat-file -p` on a commit does.

Identity resolution is a four-step fallback chain, implemented in `src/commit/identity.cpp`. The `.git/config` reader
accepts both `user.name = x` and bare `name = x` inside a `[user]` section, and skips `#` and `;` comments. Timezone
comes from comparing `localtime_r` and `gmtime_r` results.

`Environment::enclosing` is a `shared_ptr`, which is what lets a `LoxFunction`-style closure model work for methods —
there isn't one here, but the same lifetime discipline applies to `GitRepository::getGitDir`.

## The repository skeleton

`GitRepository::createSkeleton(root)` writes:

```
.git/objects/
.git/refs/heads/
.git/refs/tags/
.git/HEAD          ref: refs/heads/main
.git/config        [core] repositoryformatversion = 0, filemode, bare, logallrefupdates
.git/description
```

That is all. There is no `index`, no `hooks/`, no `info/`, no `logs/`, no `packed-refs`.

`createSkeleton` is separate from `init` so that `clone` can lay down a repository without printing a misleading
"Initialized" message.

`getGitDir` throws if `.git` is missing or is not a directory. There is no upward walk, so running from a subdirectory
fails rather than finding the repository root.

## Clone internals

`GitClone::clone` in `src/clone/clone_operation.cpp` is the orchestrator, and it is a straight-line function.

**Transport.** `GitHttp::fetchRefAdvertisement` and `fetchPack` both shell out to `curl` through
`GitUtil::runProcess`, which is `fork()` + `execvp()` with no shell. The POST body has to go through a temporary file
because it is binary.

`parseUrl` returns an `Endpoint` struct `{scheme, host, port, path}` and accepts only `http` and `https`. It splits the
authority on its **last** colon, so a bracketed IPv6 literal would mis-parse. `schemeFor(port)` maps 443 to `https` and
everything else to `http`, which is a small inversion hazard on port 443.

**pkt-line.** `readLines` parses a 4-hex-digit length that includes the four digits themselves. `0000` is a flush and is
skipped; `0001` is a delimiter and ends the ref list; any length under 4 that is not one of those, and any truncated
line, ends parsing. There is no writer — `buildUploadPackRequest` formats the lengths inline.

**Ref advertisement.** Capability parsing keeps `symref=HEAD:<target>` and discards the rest. Default branch selection
tries the symref target, then `main`, then `master`, then the first `refs/heads/*`.

**Thin packs.** After the first pack is read, `stats.missingBases` is deduplicated and fed back as `have` lines in a new
upload-pack request, at most five rounds. After that it throws, naming every hash it could not obtain.

## The pack reader

`GitPack::PackReader` is the most substantial piece of the project.

Constants worth knowing: header size 12, raw hash length 20, maximum delta depth 100, inflate chunk 8192, and a 1 GiB
ceiling on any single object's inflated size.

`walk()` indexes and inflates every entry first; `read()` then iterates in pack order and materialises each object.
Splitting it that way means a delta base that appears later in the pack is already available.

`materialize()` resolves `OFS_DELTA` against `byOffset_` and `REF_DELTA` against `byHash_` and then the loose object
store. Results are cached in both maps, so a base referenced by several deltas is inflated once.

Every object that resolves is written out as a **loose** object, so the rest of the implementation never has to know
that packs exist.

`Stats` carries `objects`, `baseObjects`, `deltaObjects`, `deltasResolved`, `objectsWritten`, `objectsFailed`,
`failures`, and `missingBases`. `clone` reads the last one; the tests assert on all of them.

## Worktree checkout

`GitWorktree::checkoutHead` resolves `HEAD` to a commit, reads the tree, and recurses.

`resolveHeadCommit` follows up to 8 `ref:` hops and then accepts a bare hash, so a detached `HEAD` works.

Per entry: a directory is created and recursed into; mode `120000` becomes `create_symlink(blob.body, path)` with a
warning if it fails; anything else is written verbatim in binary, and `100755` additionally gets the executable bits
applied.

A `Report` accumulates `branch`, `commit`, `tree`, `directories`, `files`, `symlinks`, and `missing`. If `missing` is
non-empty, `checkoutHead` throws naming every missing object rather than writing empty files. That is deliberate: a
half-written worktree that looks complete is much worse than a failed clone.

## Error handling

Everything throws `GitError::GitError`, which subclasses `std::runtime_error`. There are no error codes, no error enums,
and no `Result` type.

Several messages are in Punjabi, in keeping with the house style:

```cpp
throw GitError::GitError("Hash taqdeer naal short hai");
throw GitError::GitError("Delta vich 0 byte da instruction mila");
```

## Conventions

- **Bilingual headers.** Every non-trivial `src/` file opens with a Punjabi line and an English gloss.
- **Every header has a matching `.cpp`, with one exception.** `utils/error.hpp` is declaration-only; it exists so
  `GitError` can be thrown from anywhere without a circular include. It is documented as such rather than being an
  oversight.
- **A 500-line budget on tests,** enforced by `tests/check_size.py`. Nothing enforces a limit on `src/`.
- **`GitRepository` takes an explicit `root` on every call.** There is no ambient current-repository state.
- **Print only to stdout or stderr, never both.** The banner and all diagnostics go to stderr so stdout stays pipeable.
