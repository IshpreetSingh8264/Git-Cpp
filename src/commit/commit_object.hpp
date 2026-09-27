#pragma once

// ==========================================
// COMMIT OBJECTS DA JADU
// (The magic of commit objects)
// ==========================================

#include "commit/identity.hpp"

#include <filesystem>
#include <string>

namespace GitCommit {

/**
 * Commit object banaa
 * (Create a commit object)
 *
 * @param root      - repository da root
 * @param treeHash  - tree object da hash
 * @param parentHash - parent commit da hash, pehle commit lai khaali
 *                     (the parent's hash, empty for the first commit)
 * @param message   - commit message
 * @param author    - kaun likhya
 *                    (who wrote it)
 * @param committer - nahi dasso te author hi samajh lo
 *                   (defaults to the author when not given)
 * @return naya commit da hash
 */
std::string createCommit(const std::filesystem::path& root,
                         const std::string& treeHash,
                         const std::string& parentHash,
                         const std::string& message,
                         const Identity& author,
                         const Identity& committer = {});

/**
 * Commit object da content padho (header ke bina)
 * (Read a commit object's content, header stripped)
 */
std::string readCommit(const std::filesystem::path& root, const std::string& hash);

/**
 * Commit print karo - git cat-file commit vaang
 * (Print a commit - like git cat-file commit)
 */
void printCommit(const std::filesystem::path& root, const std::string& hash);

/**
 * Commit vichon tree hash nikaalo
 * (Pull the tree hash out of a commit)
 */
std::string getCommitTree(const std::filesystem::path& root, const std::string& hash);

} // namespace GitCommit
