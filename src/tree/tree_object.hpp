#pragma once

// ==========================================
// TREE OBJECTS DA KHAJANA
// (The treasure of tree objects)
// ==========================================

#include <filesystem>
#include <string>
#include <vector>

namespace GitTree {

/**
 * Tree entry - har file ya directory di jaankari
 * (Tree entry - what we know about one file or directory)
 */
struct TreeEntry {
    std::string mode;      // 100644 | 100755 | 40000 | 120000
    std::string name;      // file ya directory da naam
    std::string hash;      // 40 character SHA-1

    TreeEntry(std::string m, std::string n, std::string h)
        : mode(std::move(m)), name(std::move(n)), hash(std::move(h)) {}

    /**
     * Entry directory hai ya file?
     * (Is this entry a directory?)
     */
    bool isDirectory() const { return mode == "40000" || mode == "040000"; }

    /**
     * Executable bit lagi hai?
     * (Does the executable bit sit on this entry?)
     */
    bool isExecutable() const { return mode == "100755"; }

    /**
     * Symlink hai? (mode 120000, body nu hi target samajhda hai)
     * (A symlink? mode 120000, its body is the target path)
     */
    bool isSymlink() const { return mode == "120000"; }

    /**
     * Git ls-tree vich dikhanda type - "tree" ya "blob"
     * (The type word git ls-tree prints - "tree" or "blob")
     */
    std::string typeName() const { return isDirectory() ? "tree" : "blob"; }
};

/**
 * Tree object da content entries vich kholo
 * (Split a tree object's content into entries)
 *
 * Har entry aise hundi hai: "<mode> <name>\0" + 20 byte binary SHA-1
 * (Each entry is: "<mode> <name>\0" + a 20-byte binary SHA-1)
 *
 * @param content - tree object da body, header ke bina
 *                 (the tree object's body, header stripped)
 */
std::vector<TreeEntry> parseTree(const std::string& content);

/**
 * Entries toh binary tree content banao
 * (Build the binary tree content from entries)
 */
std::string serializeTree(const std::vector<TreeEntry>& entries);

/**
 * Tree print karo - git ls-tree vaang
 * (Print a tree - like git ls-tree)
 *
 * @param root - repository da root
 * @param hash - tree object da hash
 * @param name_only - sirf naam print karna hai?
 *                   (print only the names?)
 */
void printTree(const std::filesystem::path& root,
               const std::string& hash,
               bool name_only = false);

/**
 * Directory toh tree object banaa, recursively
 * (Build a tree object from a directory, recursively)
 *
 * @param root - repository da root, objects yahan likhne ne
 *              (the repository root, where objects are written)
 * @param path - directory jithon tree banana hai
 *              (the directory to snapshot)
 */
std::string writeTree(const std::filesystem::path& root,
                      const std::filesystem::path& path);

} // namespace GitTree
