#pragma once

// ==========================================
// REPOSITORY DA GHAR - .git DI STRUCTURE
// (The repository's home - the .git layout)
// ==========================================

#include <filesystem>
#include <string>

namespace GitRepository {

/**
 * .git directory da path, agar current directory repository hai
 * (Path of the .git directory, if the current directory is a repository)
 *
 * @param root - repository da working tree root
 *              (the repository's working tree root)
 * @throws GitError agar .git nahi hai ya directory nahi hai
 *         (throws GitError if .git is missing or is not a directory)
 */
std::filesystem::path getGitDir(const std::filesystem::path& root = ".");

/**
 * Kya eh directory ek Git repository hai?
 * (Is this directory a Git repository?)
 */
bool isGitRepository(const std::filesystem::path& root = ".");

/**
 * Objects directory - har object yahan store hunda hai
 * (Objects directory - every object is stored here)
 */
std::filesystem::path getObjectsDir(const std::filesystem::path& root = ".");

/**
 * Refs directory - branches te tags ke liye
 * (Refs directory - for branches and tags)
 */
std::filesystem::path getRefsDir(const std::filesystem::path& root = ".");

/**
 * HEAD file da content - "ref: refs/heads/main" ya seedha hash
 * (Content of the HEAD file - "ref: refs/heads/main" or a bare hash)
 */
std::string readHEAD(const std::filesystem::path& root = ".");

/**
 * Current branch da naam. Detached HEAD te hash return kardi hai.
 * (Current branch name. On a detached HEAD it returns the hash.)
 */
std::string getCurrentBranch(const std::filesystem::path& root = ".");

/**
 * HEAD jado kehta hai ohda commit hash nikaal do
 * (Work out which commit HEAD points at)
 *
 * Symbolic HEAD te usde ref file nu padhda hai, phir value nu
 * resolve karda hai (value khud bhi "ref: ..." ho sakdi hai).
 * Detached HEAD te seedha hash return karda hai.
 * (Reads the ref file a symbolic HEAD points at, then resolves that
 *  value, which may itself be "ref: ...". A detached HEAD is returned
 *  as-is.)
 */
std::string resolveHeadCommit(const std::filesystem::path& root = ".");

/**
 * .git da poora structure banaa, kuch print nahi karda
 * (Create the whole .git layout, printing nothing)
 *
 * Yahan koi message nahi, kyunki `clone` vich eh chalde haan te
 * "Initialized git directory" galat message lagega
 * (No message here, because `clone` calls this and "Initialized git
 *  directory" would be a misleading thing to say)
 *
 * @throws GitError agar koi file likhi na ja sakey
 */
void createSkeleton(const std::filesystem::path& root = ".");

/**
 * Nayi repository initialize karo
 * (Initialize a new repository)
 *
 * Sab folders create_directories naal bante ne, is liye eh
 * rerunnable hai - maujood repository te "Reinitialized" likhda hai.
 * (Every directory is created with create_directories, so this is
 *  rerunnable. On an existing repository it says "Reinitialized".)
 *
 * @return true on success
 */
bool init(const std::filesystem::path& root = ".");

} // namespace GitRepository
