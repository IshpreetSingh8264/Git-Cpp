#!/usr/bin/env bash
#
# Integration test: clone a repository whose HEAD is refs/heads/trunk.
#
# Why this specific shape matters: a clone that blindly assumes the remote's
# default branch is `main` or `master` passes every CodeCrafters stage, because
# the course's own fixture always uses one of those. `trunk` is neither, so
# this is the only test in the repo that can catch that assumption. It is the
# reason this test exists.
#
# The server is git's real git-http-backend (see localserve.py), not a mock, so
# pkt-line framing, the ref advertisement, status codes and the response
# Content-Type all come from git itself.
#
# Usage: clone_trunk_test.sh <path-to-git-binary> [scratch-dir]

set -uo pipefail

GIT_BIN="${1:?usage: clone_trunk_test.sh <path-to-git-binary> [scratch-dir]}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCRATCH="${2:-$(mktemp -d -t gittest-trunk-XXXXXX)}"
PORT="${GIT_TEST_PORT:-$(python3 -c 'import socket;s=socket.socket();s.bind(("127.0.0.1",0));print(s.getsockname()[1]);s.close()')}"

pass=0
fail=0

ok()   { pass=$((pass+1)); printf 'ok   %s\n' "$1"; }
bad()  { fail=$((fail+1)); printf 'FAIL %s\n     %s\n' "$1" "${2:-}"; }
check(){ if [ "$2" = "$3" ]; then ok "$1"; else bad "$1" "got [$2] want [$3]"; fi; }
check_contains(){ case "$2" in *"$3"*) ok "$1";; *) bad "$1" "[$2] does not contain [$3]";; esac; }

SRV_PID=""
cleanup() {
  [ -n "$SRV_PID" ] && kill "$SRV_PID" 2>/dev/null
  wait "$SRV_PID" 2>/dev/null
  return 0
}
trap cleanup EXIT

rm -rf "$SCRATCH"
mkdir -p "$SCRATCH"
cd "$SCRATCH" || exit 1

# ---------------------------------------------------------------------------
# 1. Build a source repository on a branch called `trunk`.
# ---------------------------------------------------------------------------
echo "--- building a source repo whose default branch is trunk ---"
git init -q -b trunk src
(
  cd src
  git config user.email "tester@example.com"
  git config user.name  "Tester"
  mkdir -p docs nested/deeper
  # A large, highly compressible file: if the clone transfers it correctly the
  # object store has to be right, not just the plumbing.
  python3 -c '
import sys
with open("log.txt", "w") as fh:
    for i in range(4000):
        fh.write("line %d: the quick brown fox jumps over the lazy dog\n" % i)
'
  echo "# readme" > docs/readme.md
  echo "deep"    > nested/deeper/deep.txt
  printf 'no trailing newline' > nonl.txt
  git add -A
  git commit -q -m "first"
  # A second commit so the clone has history to walk.
  echo "more" >> docs/readme.md
  git add -A
  git commit -q -m "second"
)

# Bare-ise it. The bare repo's HEAD must end up on refs/heads/trunk.
git clone -q --bare src trunk.git
git --git-dir=trunk.git symbolic-ref HEAD >/dev/null

check "the bare repo's HEAD is refs/heads/trunk" \
      "$(git --git-dir=trunk.git symbolic-ref HEAD)" "refs/heads/trunk"
check "the bare repo has no refs/heads/main" \
      "$(git --git-dir=trunk.git rev-parse --verify --quiet refs/heads/main >/dev/null && echo yes || echo no)" "no"
check "the bare repo has no refs/heads/master" \
      "$(git --git-dir=trunk.git rev-parse --verify --quiet refs/heads/master >/dev/null && echo yes || echo no)" "no"

# ---------------------------------------------------------------------------
# 2. Serve it over smart HTTP using git's own CGI backend.
# ---------------------------------------------------------------------------
echo
echo "--- serving trunk.git on 127.0.0.1:$PORT ---"
if ! python3 "$HERE/localserve.py" "$SCRATCH/trunk.git" "$PORT" >server.log 2>&1; then
  bad "localserve.py starts" "see $SCRATCH/server.log"
  cat server.log
  exit 1
fi &
SRV_PID=$!

ready=0
for _ in $(seq 1 50); do
  if (exec 3<>/dev/tcp/127.0.0.1/$PORT) 2>/dev/null; then ready=1; break; fi
  sleep 0.1
done
if [ "$ready" != 1 ]; then
  bad "the test server accepts connections" "nothing listening on $PORT"
  cat server.log
  exit 1
fi
ok "the test server is listening on 127.0.0.1:$PORT"

URL="http://127.0.0.1:$PORT/trunk.git"

# ---------------------------------------------------------------------------
# 3. Clone with our binary.
# ---------------------------------------------------------------------------
echo
echo "--- cloning with our binary ---"
clone_out="$("$GIT_BIN" clone "$URL" mine 2>&1)"
clone_rc=$?
printf '%s\n' "$clone_out" | sed 's/^/     | /'

if [ $clone_rc -eq 0 ]; then
  ok "clone exited 0"
else
  bad "clone exited 0" "got $clone_rc"
fi
check_contains "clone reports the cloned branch" "$clone_out" "trunk"

# ---------------------------------------------------------------------------
# 4. The clone must be a real, complete, working tree.
# ---------------------------------------------------------------------------
echo
echo "--- the resulting clone ---"
check "the clone's HEAD is refs/heads/trunk" \
      "$(git -C mine rev-parse --abbrev-ref HEAD 2>/dev/null)" "trunk"
check "the clone has a .git directory" \
      "$([ -d mine/.git ] && echo yes || echo no)" "yes"

for f in log.txt docs/readme.md nested/deeper/deep.txt nonl.txt; do
  check "the clone contains $f" "$([ -f "mine/$f" ] && echo yes || echo no)" "yes"
done

if [ -f mine/log.txt ]; then
  check "the 4000-line log.txt is byte-identical" \
        "$(cmp -s src/log.txt mine/log.txt && echo same || echo differs)" "same"
  check "log.txt has all 4000 lines" "$(wc -l < mine/log.txt | tr -d ' ')" "4000"
fi
check "a file with no trailing newline survives intact" \
      "$(cat mine/nonl.txt 2>/dev/null)" "no trailing newline"

# The real test: identical to what real git produces from the same URL.
echo
echo "--- comparing against a real git clone of the same URL ---"
git clone -q "$URL" real 2>/dev/null
if [ -d real ]; then
  ok "real git clone of the same URL succeeded (control)"
  if diff -r --exclude=.git src real >/dev/null 2>&1; then
    ok "real git's working tree matches the source"
  else
    bad "real git's working tree matches the source" "diff -r reported differences"
  fi
  if diff -r --exclude=.git real mine >/dev/null 2>&1; then
    ok "our clone is byte-identical to real git's clone"
  else
    bad "our clone is byte-identical to real git's clone" "diff -r reported differences"
    diff -r --exclude=.git real mine 2>&1 | head -20 | sed 's/^/     | /'
  fi
  ours=$(find mine/.git/objects -type f 2>/dev/null | wc -l | tr -d ' ')
  theirs=$(find real/.git/objects -type f 2>/dev/null | wc -l | tr -d ' ')
  check "the object store holds the same number of objects as real git's" \
        "$ours" "$theirs"
else
  bad "real git clone of the same URL succeeded (control)" "the control clone failed"
fi

# git fsck on our clone is the strongest cheap check available: if any object
# were missing, mis-hashed, or left half-reconstructed, it would say so.
if command -v git >/dev/null && [ -d mine/.git ]; then
  if git -C mine fsck --no-progress >fsck.log 2>&1; then
    ok "git fsck reports the clone's object store is intact"
  else
    bad "git fsck reports the clone's object store is intact" "see $SCRATCH/fsck.log"
    head -20 fsck.log | sed 's/^/     | /'
  fi
fi

echo
echo "=================================================="
printf 'TOTAL: pass=%d fail=%d\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
