#!/usr/bin/env python3
"""Runs the pack reader test against a freshly generated synthetic pack.

The pack is not checked in: it is built by make_pack.py on every run, so the
fixture can never drift from the generator that documents it. The generator
prints a manifest of every expected object hash, and the test checks the
reader's real output against that manifest.

Usage: run_pack_test.py <pack_reader_test-binary>
"""
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parent.parent


def main():
    binary = pathlib.Path(sys.argv[1]).resolve()
    scratch = pathlib.Path(tempfile.mkdtemp(prefix="packtest-"))
    try:
        pack = scratch / "synthetic.pack"
        manifest = scratch / "manifest.txt"

        gen = subprocess.run(
            [sys.executable, str(HERE / "make_pack.py"), str(pack), str(manifest)],
            capture_output=True, text=True,
        )
        if gen.returncode != 0:
            print("FAIL make_pack.py could not build the fixture")
            print(gen.stdout + gen.stderr)
            return 1
        print(gen.stdout.strip())

        expected = dict(
            line.split("\t", 1)
            for line in manifest.read_text(encoding="utf-8").splitlines()
            if "\t" in line
        )
        # The pack header claims 7 objects; check the generator agrees with
        # itself before blaming the reader for a mismatch.
        if len(expected) < 12:
            print(f"FAIL manifest is short: {len(expected)} keys")
            return 1

        proc = subprocess.run([str(binary), str(pack), str(manifest), str(scratch / "store")])
        return proc.returncode
    finally:
        shutil.rmtree(scratch, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
