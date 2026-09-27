// Feeds a synthetic pack to the real GitPack::PackReader and checks the outcome
// against a manifest with a known expected result.
//
// The pack is built by make_pack.py and deliberately contains one object for
// each delta path:
//
//   A  raw blob
//   B  OFS_DELTA on A
//   C  REF_DELTA on B             (base is itself a delta result)
//   D  raw blob
//   E  REF_DELTA on D             (base is in the pack)
//   F  REF_DELTA on a base nobody has anywhere
//   G  REF_DELTA on a base that lives only in the object store
//
// CASE A -- thin pack. The object store is pre-seeded with G's base, so G must
//          resolve and be written byte for byte.
// CASE B -- empty object store. The same pack, nothing pre-seeded. G's base is
//          now missing, so G must NOT be written and the base hash must appear
//          in stats.missingBases so the caller can go and ask for it. F's base
//          is unresolvable in both cases and must always be reported.
//
// Usage: pack_reader_test <pack> <manifest> <scratch-dir>

#include "clone/pack_reader.hpp"
#include "objects/object_store.hpp"
#include "repository/repository.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

static int fails = 0;

static void ok(const std::string& what) {
    std::cout << "ok   " << what << "\n";
}

static void bad(const std::string& what, const std::string& detail) {
    std::cout << "FAIL " << what << "\n     " << detail << "\n";
    ++fails;
}

static void check(bool cond, const std::string& what, const std::string& detail = "") {
    if (cond) {
        ok(what);
    } else {
        bad(what, detail);
    }
}

static std::string unbase64(const std::string& in) {
    std::string out;
    int value = 0, bits = 0;
    for (char c : in) {
        int digit = (c >= 'A' && c <= 'Z') ? c - 'A'
                 : (c >= 'a' && c <= 'z') ? c - 'a' + 26
                 : (c >= '0' && c <= '9') ? c - '0' + 52
                 : (c == '=') ? -2 : -1;
        if (digit == -1) continue;
        if (digit == -2) break;
        value = (value << 6) | digit;
        bits += 6;
        if (bits >= 8) { bits -= 8; out.push_back(char((value >> bits) & 0xFF)); }
    }
    return out;
}

static std::map<std::string, std::string> readManifest(const std::string& path) {
    std::map<std::string, std::string> manifest;
    std::ifstream man(path);
    std::string line;
    while (std::getline(man, line)) {
        size_t tab = line.find('\t');
        if (tab != std::string::npos) {
            manifest[line.substr(0, tab)] = line.substr(tab + 1);
        }
    }
    return manifest;
}

static std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

static std::string bodyOf(const std::filesystem::path& root, const std::string& hash) {
    return GitObject::splitObject(GitObject::readObject(root, hash)).body;
}

// Objects that must land in the store in both cases: A and D are raw, B and C
// are the delta chain, E is a REF_DELTA whose base is in the pack. F is
// deliberately absent -- its base exists nowhere.
static void checkCommon(const std::filesystem::path& root,
                        const std::map<std::string, std::string>& m,
                        const char* label) {
    for (const char* name : {"A", "B", "C", "D", "E"}) {
        check(GitObject::hasObject(root, m.at(name)),
              std::string(label) + ": " + name + " (" + m.at(name) + ") was written",
              "object is absent from the store");
    }
    check(bodyOf(root, m.at("E")) == unbase64(m.at("E_body")),
          std::string(label) + ": E's reconstructed body is byte-for-byte correct");
    check(!bodyOf(root, m.at("B")).empty() && !bodyOf(root, m.at("C")).empty(),
          std::string(label) + ": no delta result came out empty");

    // F's base is unresolvable, so F must not be written -- an empty or
    // half-reconstructed file is exactly the bug this case exists to catch.
    check(!GitObject::hasObject(root, m.at("F")),
          std::string(label) + ": F was NOT written (its base exists nowhere)",
          "object " + m.at("F") + " should not exist");
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "usage: pack_reader_test <pack> <manifest> <scratch-dir>\n";
        return 2;
    }
    const std::string packPath = argv[1];
    const auto manifest = readManifest(argv[2]);
    const std::filesystem::path scratch = argv[3];
    const std::string packBytes = readFile(packPath);

    // ------------------------------------------------------------- CASE A
    std::cout << "--- CASE A: thin pack (object store pre-seeded) ---\n";
    {
        std::filesystem::path root = scratch / "caseA";
        std::filesystem::remove_all(root);
        GitRepository::createSkeleton(root);

        const std::string base = GitObject::writeObject(
            root, "blob", unbase64(manifest.at("store_base_body")));
        check(base == manifest.at("store_base_hash"),
              "A: the seeded store base hashes to the manifest value",
              "got " + base + " want " + manifest.at("store_base_hash"));

        const std::string only = GitObject::writeObject(
            root, "blob", unbase64(manifest.at("store_only_body")));
        check(only == manifest.at("store_only_hash"),
              "A: the store-only base hashes to the manifest value",
              "got " + only + " want " + manifest.at("store_only_hash"));

        GitPack::Stats stats = GitPack::PackReader(root, packBytes).read();
        std::cout << "     pack objects=" << stats.objects
                  << " raw=" << stats.baseObjects
                  << " delta=" << stats.deltaObjects
                  << " resolved=" << stats.deltasResolved
                  << " written=" << stats.objectsWritten
                  << " failed=" << stats.objectsFailed << "\n";
        for (const auto& f : stats.failures) std::cout << "     failure: " << f << "\n";

        checkCommon(root, manifest, "A");

        check(GitObject::hasObject(root, manifest.at("G")),
              "A: G resolved against the object store and was written");
        if (GitObject::hasObject(root, manifest.at("G"))) {
            check(bodyOf(root, manifest.at("G")) == unbase64(manifest.at("G_body")),
                  "A: G's reconstructed body is byte-for-byte correct");
        }

        check(std::find(stats.missingBases.begin(), stats.missingBases.end(),
                        manifest.at("missing_base")) != stats.missingBases.end(),
              "A: F's unresolvable base was reported in missingBases",
              "missingBases did not contain " + manifest.at("missing_base"));
    }

    // ------------------------------------------------------------- CASE B
    std::cout << "\n--- CASE B: empty object store ---\n";
    {
        std::filesystem::path root = scratch / "caseB";
        std::filesystem::remove_all(root);
        GitRepository::createSkeleton(root);

        GitPack::Stats stats = GitPack::PackReader(root, packBytes).read();
        std::cout << "     pack objects=" << stats.objects
                  << " raw=" << stats.baseObjects
                  << " delta=" << stats.deltaObjects
                  << " resolved=" << stats.deltasResolved
                  << " written=" << stats.objectsWritten
                  << " failed=" << stats.objectsFailed << "\n";
        for (const auto& f : stats.failures) std::cout << "     failure: " << f << "\n";

        checkCommon(root, manifest, "B");

        check(!GitObject::hasObject(root, manifest.at("G")),
              "B: G was NOT written (its base is not in an empty store)",
              "object " + manifest.at("G") + " should not exist");

        // The hash to report is the *base* the reader went looking for (S), not
        // G. Reporting G would tell the caller to re-fetch an object the server
        // has never heard of.
        check(std::find(stats.missingBases.begin(), stats.missingBases.end(),
                        manifest.at("store_only_hash")) != stats.missingBases.end(),
              "B: G's missing BASE hash was reported in missingBases so it can be re-fetched",
              "missingBases did not contain the base " + manifest.at("store_only_hash"));

        check(!stats.missingBases.empty() &&
                  stats.missingBases.front() == manifest.at("missing_base"),
              "B: F's base is the first missing base reported",
              stats.missingBases.empty() ? "missingBases was empty"
                                         : "first was " + stats.missingBases.front());
    }

    std::cout << "\n";
    if (fails) {
        std::cout << fails << " FAILURES\n";
    } else {
        std::cout << "all pack reader tests passed\n";
    }
    return fails ? 1 : 0;
}
