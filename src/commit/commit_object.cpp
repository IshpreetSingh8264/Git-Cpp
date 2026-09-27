#include "commit/commit_object.hpp"

#include "objects/object_store.hpp"
#include "utils/error.hpp"

#include <ctime>
#include <iostream>
#include <sstream>

namespace GitCommit {

std::string createCommit(const std::filesystem::path& root,
                         const std::string& treeHash,
                         const std::string& parentHash,
                         const std::string& message,
                         const Identity& author,
                         const Identity& committer) {
    std::ostringstream content;

    // Tree reference - zaroori hai
    // (Tree reference - required)
    content << "tree " << treeHash << "\n";

    if (!parentHash.empty()) {
        content << "parent " << parentHash << "\n";
    }

    long long now = static_cast<long long>(std::time(nullptr));
    content << "author " << formatPersonInfo(author.name, author.email, now) << "\n";

    // Committer alag hai te sirf tab alag likhho
    // (Only write a separate committer when it really is separate)
    const Identity& effective = committer.name.empty() ? author : committer;
    content << "committer " << formatPersonInfo(effective.name, effective.email, now) << "\n";

    // Khaali line, phir message - Git da format
    // (Blank line, then the message - Git's format)
    content << "\n" << message << "\n";

    return GitObject::writeObject(root, "commit", content.str());
}

std::string readCommit(const std::filesystem::path& root, const std::string& hash) {
    GitObject::Object object = GitObject::splitObject(GitObject::readObject(root, hash));
    if (object.type != "commit") {
        throw GitError::GitError("Eh commit nahi hai: " + hash + " (type " + object.type + ")");
    }
    return object.body;
}

void printCommit(const std::filesystem::path& root, const std::string& hash) {
    std::string body = readCommit(root, hash);
    std::cout << "commit " << hash << "\n";
    std::cout << body;
    std::cout.flush();
}

std::string getCommitTree(const std::filesystem::path& root, const std::string& hash) {
    std::string body = readCommit(root, hash);

    // "tree " line shuru vich hundi hai, header da pehla line
    // (The "tree " line is the commit header's first line)
    if (body.rfind("tree ", 0) != 0) {
        throw GitError::GitError("Commit da pehla line 'tree <hash>' nahi hai: " + hash);
    }

    size_t end = body.find('\n');
    if (end == std::string::npos) {
        throw GitError::GitError("Commit vich tree line adhoori hai: " + hash);
    }
    return body.substr(5, end - 5);
}

} // namespace GitCommit
