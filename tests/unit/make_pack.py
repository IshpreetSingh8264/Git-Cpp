#!/usr/bin/env python3
"""Builds a pack file with a known shape so the reader's behaviour can be checked
byte for byte. Emits the pack plus the expected object hashes as a manifest.

The pack exercises every delta path the reader has to get right:

    A  raw blob
    B  OFS_DELTA on A
    C  REF_DELTA on B            (base is itself a delta result)
    D  raw blob
    E  REF_DELTA on D            (base is in the pack)
    F  REF_DELTA on a hash nobody has   -> must NOT be written, base reported
    G  REF_DELTA on a base that lives only in the object store
    S  that store-only base itself, never in the pack

F and G are the interesting ones: they are the thin-pack case. The reader must
report F's base as missing instead of writing a broken object, and must resolve
G when the base happens to be in the store.

Usage: make_pack.py <out.pack> <out.manifest>
"""
import base64
import hashlib
import struct
import sys
import zlib

def git_hash(kind, body):
    return hashlib.sha1(b"%s %d\0%s" % (kind.encode(), len(body), body)).hexdigest()

def varint(n):
    out = bytearray()
    while True:
        b = n & 0x7F
        n >>= 7
        if n:
            out.append(b | 0x80)
        else:
            out.append(b)
            return bytes(out)

def obj_header(kind, size):
    """3 type bits + variable length size, as a pack object header."""
    out = bytearray()
    b = (kind << 4) | (size & 0x0F)
    size >>= 4
    while size:
        out.append(b | 0x80)
        b = size & 0x7F
        size >>= 7
    out.append(b)
    return bytes(out)

def ofs_encode(back):
    out = bytearray([back & 0x7F])
    back >>= 7
    while back:
        back -= 1
        out.insert(0, 0x80 | (back & 0x7F))
        back >>= 7
    return bytes(out)

def literal_insert(data):
    out = bytearray()
    for i in range(0, len(data), 127):
        chunk = data[i:i + 127]
        out.append(len(chunk))
        out += chunk
    return bytes(out)

def make_delta(base, result, copy_head):
    """Copy the first copy_head bytes of base, then insert the rest verbatim."""
    assert base[:copy_head] == result[:copy_head], "copy prefix must match"
    out = bytearray()
    out += varint(len(base))
    out += varint(len(result))
    # copy instruction: offset 0, size as a 16-bit little endian value.
    # Size bits 4, 5 and 6 of the opcode say how many size bytes follow.
    if copy_head < 0x100:
        out.append(0x80 | 0x10)
        out.append(copy_head)
    else:
        out.append(0x80 | 0x10 | 0x20)
        out += struct.pack('<H', copy_head)
    out += literal_insert(result[copy_head:])
    return bytes(out)

def entry(kind, body):
    return obj_header(kind, len(body)) + zlib.compress(body)

def ofs_delta(kind, body, back):
    return obj_header(6, len(body)) + ofs_encode(back) + zlib.compress(body)

def ref_delta(kind, body, base_hash):
    return obj_header(7, len(body)) + bytes.fromhex(base_hash) + zlib.compress(body)

def build():
    blobs = {}
    base = b"The quick brown fox jumps over the lazy dog.\n" * 60
    mid = base.replace(b"over the lazy dog", b"over the quick dog", 1)  # early edit
    top = mid + b"appended line\n"                     # plus an appended line
    alien = bytes.fromhex("de" * 20)                    # a base nobody will have
    store_base = b"object that lives only in the object store\n" * 3
    store_only = b"this base is never in the pack, only in the object store\n" * 4

    blobs["A"] = ("blob", base)
    blobs["B"] = ("blob", mid)     # OFS_DELTA on A
    blobs["C"] = ("blob", top)     # REF_DELTA on B
    blobs["D"] = ("blob", store_base)  # raw, and also the store-only base
    blobs["E"] = ("blob", store_base + b"one more\n")  # REF_DELTA on D, in pack
    blobs["F"] = ("blob", base[::-1])                   # REF_DELTA on a base nobody has
    blobs["G"] = ("blob", store_only + b"patched\n")    # REF_DELTA on a store-only base
    blobs["S"] = ("blob", store_only)                   # the store-only base itself

    hashes = {name: git_hash(kind, body) for name, (kind, body) in blobs.items()}

    # F's base: a hash that exists nowhere, to prove missing bases get reported
    alien_hash = git_hash("blob", alien)

    body = bytearray(b"PACK" + struct.pack(">II", 2, 7))

    a_off = len(body)
    body += entry(3, base)
    b_off = len(body)
    body += ofs_delta(3, make_delta(base, mid, 20), b_off - a_off)
    c_off = len(body)
    body += ref_delta(3, make_delta(mid, top, 20), hashes["B"])
    d_off = len(body)
    body += entry(3, store_base)
    e_off = len(body)
    body += ref_delta(3, make_delta(store_base, store_base + b"one more\n", 20), hashes["D"])
    f_off = len(body)
    body += ref_delta(3, make_delta(base[::-1], base[::-1] + b"tail\n", len(base)), alien_hash)
    body += ref_delta(3, make_delta(store_only, store_only + b"patched\n", 20), hashes["S"])

    manifest = {
        "A": hashes["A"], "B": hashes["B"], "C": hashes["C"],
        "D": hashes["D"], "E": hashes["E"], "F": hashes["F"],
        "store_base_hash": hashes["D"],
        "store_base_body": base64.b64encode(store_base).decode(),
        "missing_base": alien_hash,
        "G": hashes["G"],
        "store_only_hash": hashes["S"],
        "store_only_body": base64.b64encode(store_only).decode(),
        "G_body": base64.b64encode(store_only + b"patched\n").decode(),
        "E_body": base64.b64encode(store_base + b"one more\n").decode(),
    }
    return bytes(body), manifest

if __name__ == "__main__":
    pack, manifest = build()
    open(sys.argv[1], "wb").write(pack)
    with open(sys.argv[2], "w") as fh:
        for key, value in manifest.items():
            fh.write("%s\t%s\n" % (key, value))
    print("wrote %s (%d bytes)" % (sys.argv[1], len(pack)))
