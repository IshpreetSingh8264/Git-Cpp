#include "repository/repository.hpp"

#include "utils/error.hpp"

#include <fstream>
#include <iostream>

namespace GitRepository {

namespace {

const char* const kRefPrefix = "ref: refs/heads/";

std::string readFirstLine(const std::filesystem::path& path, const char* what) {
    std::ifstream file(path);
    if (!file) {
        throw GitError::GitError(std::string(what) + " nahi padhi ja sakdi: " + path.string());
    }
    std::string line;
    std::getline(file, line);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
        line.pop_back();
    }
    return line;
}

void writeFile(const std::filesystem::path& path, const std::string& contents) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw GitError::GitError("File likh nahi sakdi: " + path.string());
    }
    file << contents;
}

} // namespace

std::filesystem::path getGitDir(const std::filesystem::path& root) {
    std::filesystem::path gitDir = root / ".git";
    if (!std::filesystem::exists(gitDir) || !std::filesystem::is_directory(gitDir)) {
        throw GitError::GitError("Eh Git repository nahi hai: " + root.string());
    }
    return gitDir;
}

bool isGitRepository(const std::filesystem::path& root) {
    std::error_code ec;
    std::filesystem::path gitDir = root / ".git";
    return std::filesystem::exists(gitDir, ec) && std::filesystem::is_directory(gitDir, ec);
}

std::filesystem::path getObjectsDir(const std::filesystem::path& root) {
    return getGitDir(root) / "objects";
}

std::filesystem::path getRefsDir(const std::filesystem::path& root) {
    return getGitDir(root) / "refs";
}

std::string readHEAD(const std::filesystem::path& root) {
    return readFirstLine(getGitDir(root) / "HEAD", "HEAD");
}

std::string getCurrentBranch(const std::filesystem::path& root) {
    std::string head = readHEAD(root);
    if (head.rfind(kRefPrefix, 0) == 0) {
        return head.substr(std::char_traits<char>::length(kRefPrefix));
    }
    // Detached HEAD - koi branch nahi, seedha hash
    // (Detached HEAD - no branch, just the hash)
    return head;
}

std::string resolveHeadCommit(const std::filesystem::path& root) {
    std::filesystem::path gitDir = getGitDir(root);

    // Symbolically as many ref hops as we like: "ref: a" -> "ref: b" -> hash
    // (Symbolic ref, as many hops as we like)
    std::string value = readHEAD(root);
    int guard = 0;
    while (value.rfind("ref:", 0) == 0 && guard < 8) {
        std::string refName = value.substr(4);
        while (!refName.empty() && refName.front() == ' ') {
            refName.erase(refName.begin());
        }
        value = readFirstLine(gitDir / refName, "Ref");
        ++guard;
    }

    if (value.empty()) {
        throw GitError::GitError("HEAD kuch nahi point karda: " + root.string());
    }
    return value;
}

void createSkeleton(const std::filesystem::path& root) {
    // create_directories recursive hai te duplicate te koi error nahi dinda,
    // is liye eh re-init te bhi chal janda hai
    // (create_directories is recursive and silent about duplicates, so
    //  this also survives a re-init)
    std::filesystem::create_directories(root / ".git" / "objects");
    std::filesystem::create_directories(root / ".git" / "refs" / "heads");
    std::filesystem::create_directories(root / ".git" / "refs" / "tags");

    writeFile(root / ".git" / "HEAD", "ref: refs/heads/main\n");
    writeFile(root / ".git" / "config",
              "[core]\n"
              "\trepositoryformatversion = 0\n"
              "\tfilemode = true\n"
              "\tbare = false\n"
              "\tlogallrefupdates = true\n");
    writeFile(root / ".git" / "description",
              "Unnamed repository; edit this file 'description' to name the repository.\n");
}

bool init(const std::filesystem::path& root) {
    try {
        bool alreadyThere = isGitRepository(root);
        createSkeleton(root);

        std::cout << (alreadyThere ? "Reinitialized git directory\n"
                                   : "Initialized git directory\n");
        return true;

    } catch (const GitError::GitError& e) {
        std::cerr << "Repository init vich problem: " << e.what() << "\n";
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Koi masla aa gaya: " << e.what() << "\n";
        return false;
    }
}

} // namespace GitRepository
