# Tests

Local test infrastructure for Punjabi Git. **No network** — the clone suite
serves a local repository through git's own `git-http-backend` on `127.0.0.1`.

## Run everything

```sh
./tests/run.sh
```

Configures, builds, runs every suite, exits non-zero if anything fails.

| Command | What it runs |
|---|---|
| `./tests/run.sh` | build + both suites |
| `./tests/run.sh pack` | the synthetic-pack reader suite only |
| `./tests/run.sh clone` | the trunk-branch clone suite only |

`BUILD_DIR` and `PYTHON` are honoured if set. `ctest` also works:

```sh
cmake -B build -S . && cmake --build build && ctest --test-dir build --output-on-failure
```

Requires `git` (for `init`/`clone`/`fsck` as an oracle) and python3. No test
framework, no new dependencies.

## What is here

```
tests/
  run.sh                          the entrypoint
  check_size.py                   fails if a suite file passes 500 lines
  unit/
    make_pack.py                  builds the synthetic pack + expected-hash manifest
    pack_reader_test.cpp          drives the real GitPack::PackReader
    run_pack_test.py              generates the fixture, then runs the test
  integration/
    localserve.py                 smart HTTP via git's own CGI backend
    clone_trunk_test.sh           clones a repo whose HEAD is refs/heads/trunk
```

### `unit/` — synthetic pack, 25 assertions

This is the part of the repo the 7 CodeCrafters stages do **not** cover, and it
exists because of a bug that already happened: a "skip the delta object" path
made a clone quietly lose blobs while the stage still passed.

`make_pack.py` builds a pack byte by byte — pack header, 3-bit type plus
variable-length size, `OFS_DELTA`'s backwards offset encoding, `REF_DELTA`'s
base hash, and hand-built delta copy/insert opcodes — containing one object per
code path the reader has to get right:

| | shape | why it is there |
|---|---|---|
| `A` | raw blob | the trivial case |
| `B` | `OFS_DELTA` on `A` | offset arithmetic, and the base is earlier in the pack |
| `C` | `REF_DELTA` on `B` | the base is itself a delta result, so resolution must recurse |
| `D` | raw blob | |
| `E` | `REF_DELTA` on `D` | base is in the pack, non-delta |
| `F` | `REF_DELTA` on a hash that exists **nowhere** | must **not** be written; its base must be reported |
| `G` | `REF_DELTA` on a base that lives **only in the object store** | the thin-pack case |

The generator also writes a manifest of every expected SHA-1 and every expected
body, so the assertions are "this object must exist with exactly this hash and
exactly these bytes", not "the reader returned something".

The pack is **not** checked in. It is regenerated on every run, so the fixture
can never drift away from the generator that documents it.

**CASE A — thin pack.** The object store is pre-seeded with `G`'s base. `G` must
resolve out of the store, be written, and its reconstructed body must match
byte for byte. `F` must still be absent and its base reported.

**CASE B — empty object store.** The same pack, nothing pre-seeded. `G`'s base
is now missing, so `G` must **not** be written — and the hash reported in
`missingBases` must be the hash of the **base** it went looking for, not of `G`
itself. Reporting `G` would tell the caller to re-fetch an object the server
has never heard of. That distinction is asserted directly.

Both cases also check that `E`'s reconstructed body is byte-for-byte correct and
that no delta result came out empty.

### `integration/` — clone a repo on `trunk`, 20 assertions

**This is the reason the suite exists.** A clone that assumes the remote's
default branch is `main` or `master` passes all 7 CodeCrafters stages, because
the course fixture uses one of those names. This test serves a repository whose
`HEAD` is `refs/heads/trunk` — neither `main` nor `master`, and the suite asserts
both are genuinely absent so the test cannot silently rot into a `main` test.

The server is git's real `git-http-backend` via CGI, not a hand-rolled
responder. That matters: pkt-line framing, the ref advertisement and its flush
packets, the `Content-Type` of the upload-pack response and the status codes all
come from git itself. A mock would only prove the mock works.

The assertions:

- the bare repo's `HEAD` is `refs/heads/trunk`, and `main`/`master` do not exist
- `clone` exits 0 and names the branch it cloned
- the clone's `HEAD` is `refs/heads/trunk`, not detached, not `master`
- a 4000-line file survives byte-identical (`cmp`), a nested directory survives,
  and a file with **no trailing newline** survives intact
- a real `git clone` of the same URL is taken as a **control**, then
  `diff -r --exclude=.git` between our clone and git's must be empty, and the
  two object stores must hold the same number of objects
- `git fsck` on our clone must report the object store intact

The `fsck` and `diff -r` checks are the strongest cheap assertions available:
a missing, mis-hashed or half-reconstructed object makes both fail.

## What this does NOT cover

- **`codecrafters test` is still the only authority.** It builds and runs all 7
  stages remotely. This suite is a fast local proxy for the behaviour the stages
  miss. `run.sh` passing does not mean the stages pass.
- **The local server is HTTP, not HTTPS, and speaks only smart HTTP.** No TLS,
  no authentication (`REMOTE_USER` is hardcoded to `tester`), no dumb HTTP
  fallback, no `git://`, no SSH, no redirect handling, no chunked transfer
  encoding. `http_transport` has never been tested against a server that
  challenges, redirects, or answers `401`.
- **The clone suite uses a small repository** (13 objects, no deltas — the
  server chooses the pack shape, and `upload-pack` sends everything raw for a
  repo this small). The delta paths are covered by the synthetic unit suite
  instead, not end to end over HTTP. A real delta-heavy clone was previously
  checked against `pallets/click` (13020 raw + 18638 deltas, `diff -r` empty,
  `fsck` clean) but that needs network and is **not** part of this suite.
- **A checkout that hits a missing blob is not covered here.** That behaviour
  (fails loudly, writes no file) was checked separately and is not in this repo.
  If you change `worktree_writer`, add a case for it.
- **The thin-pack follow-up fetch is not exercised end to end.** CASE A and
  CASE B verify that `PackReader` *reports* the missing base correctly. Nothing
  here proves `clone_operation` then correctly issues the `have` lines and
  completes a real thin-pack round trip. That is the highest-value gap in this
  suite.
- **No tests for `init`, `hash-object`, `cat-file`, `write-tree`, `commit-tree`
  or `ls-tree`.** `test.sh` in the repo root smoke-tests those, but it is
  interactive (it prompts on cleanup), it asserts almost nothing, and it exits 0
  even when a test fails. Do not treat it as a gate.
- **`test.sh` is not wired into `run.sh`** for that reason. Fixing it is a
  separate job from making the existing harness permanent.
- **No concurrency, no large-object, and no malformed-packet tests.** No timeout
  or retry behaviour, no `Transfer-Encoding: chunked`, no truncated or corrupt
  pack, no pack with a wrong object count in its header.
- **Only one port, one connection at a time.** The suite binds an ephemeral port
  and serves a single clone; there is no parallel-clone or keep-alive coverage.
- **Exit-code and stderr text are not asserted** for the clone, beyond exit 0.
  A regression that changes the Punjabi messages would not be caught.
