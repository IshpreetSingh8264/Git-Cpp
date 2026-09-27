#include "clone/worktree_writer.hpp"

#include "commit/commit_object.hpp"
#include "objects/object_store.hpp"
#include "repository/repository.hpp"
#include "tree/tree_object.hpp"
#include "utils/error.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <system_error>

namespace GitWorktree {

namespace {

// 040000 te 40000 dono likhi aundian ne - Git dono sambhalda hai
// (Both 040000 and 40000 show up - Git accepts either)
void applyExecutableBit(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::permissions(
        path,
        std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec |
            std::filesystem::perms::others_exec,
        std::filesystem::perm_options::add, ec);
}

void checkoutTree(const std::filesystem::path& root,
                  const std::string& treeHash,
                  const std::filesystem::path& path,
                  Report& report) {
    GitObject::Object object;
    try {
        object = GitObject::splitObject(GitObject::readObject(root, treeHash));
    } catch (const std::exception& e) {
        report.missing.push_back(treeHash + " (tree: " + e.what() + ")");
        return;
    }

    if (object.type != "tree") {
        report.missing.push_back(treeHash + " (object type " + object.type + ", tree chahida si)");
        return;
    }

    for (const GitTree::TreeEntry& entry : GitTree::parseTree(object.body)) {
        std::filesystem::path entryPath = path / entry.name;

        if (entry.isDirectory()) {
            std::error_code ec;
            std::filesystem::create_directories(entryPath, ec);
            ++report.directories;
            checkoutTree(root, entry.hash, entryPath, report);
            continue;
        }

        // Blob ya symlink dono laye object store toh hi aunde ne
        // (A blob and a symlink both come out of the object store)
        GitObject::Object blob;
        try {
            blob = GitObject::splitObject(GitObject::readObject(root, entry.hash));
        } catch (const std::exception& e) {
            // Koi object miss ho gaya. Khaali file likh ke chup na karo -
            // oh kade nu galat working tree bana dindi hai
            // (An object went missing. Do not paper over it with an empty
            //  file, that corrupts the working tree in a way nobody sees)
            report.missing.push_back(entry.hash + " (" + entry.name + ": " + e.what() + ")");
            continue;
        }

        if (entry.isSymlink()) {
            std::error_code ec;
            std::filesystem::create_symlink(blob.body, entryPath, ec);
            if (!ec) {
                ++report.symlinks;
                continue;
            }
            // Symlink nahi ban sakde (Windows, ya badha hua filesystem) -
            // chhad dein, Git vich bhi warning ditti jaandi hai
            // (Could not create the symlink (Windows, or a filesystem that
            //  has none). Leave it out, Git warns about this too)
            std::cerr << "Warning: symlink " << entryPath << " nahi ban sakda\n";
            continue;
        }

        std::ofstream out(entryPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            throw GitError::GitError("File likh nahi sakdi: " + entryPath.string());
        }
        out.write(blob.body.data(), static_cast<std::streamsize>(blob.body.size()));
        if (!out) {
            throw GitError::GitError("File likh nahi sakdi: " + entryPath.string());
        }
        out.close();

        if (entry.isExecutable()) {
            applyExecutableBit(entryPath);
        }
        ++report.files;
    }
}

} // namespace

Report checkoutCommit(const std::filesystem::path& root, const std::string& commitHash) {
    Report report;
    report.commit = commitHash;

    report.tree = GitCommit::getCommitTree(root, commitHash);
    checkoutTree(root, report.tree, root, report);
    return report;
}

Report checkoutHead(const std::filesystem::path& root) {
    std::string branch = GitRepository::getCurrentBranch(root);
    std::string commit = GitRepository::resolveHeadCommit(root);

    Report report = checkoutCommit(root, commit);
    report.branch = branch;

    if (!report.complete()) {
        // Chup na karo. Jo object miss hoye oh likha jauga, te listing de
        // dassi jaugi ki kinna data Adhoora hai
        // (No hushing. Name the missing objects and say how much of the
        //  tree is incomplete)
        std::ostringstream message;
        message << "Checkout adhoora hai: " << report.missing.size()
                << " object(s) object store vich nahi mile"
                << " (incomplete checkout: " << report.missing.size() << " object(s) missing)";
        for (const auto& item : report.missing) {
            message << "\n  " << item;
        }
        throw GitError::GitError(message.str());
    }

    return report;
}

} // namespace GitWorktree
