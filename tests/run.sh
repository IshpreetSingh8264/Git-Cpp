#!/usr/bin/env bash
#
# The one command that runs every local test in this repo.
#
#   ./tests/run.sh              # build, then run everything
#   ./tests/run.sh pack         # just the synthetic-pack reader suite
#   ./tests/run.sh clone        # just the trunk-branch clone suite
#
# Exits non-zero if any suite fails. No network: the clone suite serves a local
# repository through git's own git-http-backend on 127.0.0.1.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO/build}"
PY="${PYTHON:-python3}"

cd "$REPO"

ALL="pack clone"
SUITES="$ALL"
case "${1:-}" in
  pack|clone) SUITES="$1"; shift ;;
esac

CMAKE_ARGS=(-B "$BUILD_DIR" -S .)
if [ -n "${VCPKG_ROOT:-}" ] && [ -f "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" ]; then
  CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
fi

want() {
  case " $SUITES " in *" $1 "*) return 0 ;; *) return 1 ;; esac
}

echo "== configure =="
cmake "${CMAKE_ARGS[@]}" >/dev/null
echo "== build =="
cmake --build "$BUILD_DIR" >/dev/null

rc=0

if want pack; then
  echo
  echo "== pack reader (tests/unit/) =="
  [ -x "$BUILD_DIR/pack_reader_test" ] || { echo "FATAL: pack_reader_test not built" >&2; rc=1; }
  [ $rc -eq 0 ] && "$PY" tests/unit/run_pack_test.py "$BUILD_DIR/pack_reader_test" || rc=1
fi

if want clone; then
  echo
  echo "== clone over smart HTTP (tests/integration/) =="
  [ -x "$BUILD_DIR/git" ] || { echo "FATAL: build/git not built" >&2; rc=1; }
  [ $rc -eq 0 ] && bash tests/integration/clone_trunk_test.sh "$BUILD_DIR/git" || rc=1
fi

echo
echo "== suite size budget =="
"$PY" tests/check_size.py tests || rc=1

echo
if [ $rc -eq 0 ]; then
  echo "ALL SUITES PASSED"
else
  echo "SOME SUITES FAILED"
fi
exit $rc
