#include "tree/tree_object.hpp"

#include "objects/object_store.hpp"
#include "utils/error.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace GitTree {

namespace {

constexpr size_t kRawHashLength = 20;

std::string hexByte(unsigned char value) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.push_back(digits[value >> 4]);
    out.push_back(digits[value & 0x0F]);
    return out;
}

unsigned char fromHexNibble(char c) {
    if (c >= '0' && c <= '9') return static_cast<unsigned char>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<unsigned char>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<unsigned char>(c - 'A' + 10);
    throw GitError::GitError(std::string("Hex digit galat hai: ") + c);
}

} // namespace

std::vector<TreeEntry> parseTree(const std::string& content) {
    std::vector<TreeEntry> entries;
    size_t pos = 0;

    while (pos < content.size()) {
        size_t nullPos = content.find('\0', pos);
        if (nullPos == std::string::npos) {
            break;
        }

        std::string modeName = content.substr(pos, nullPos - pos);
        size_t spacePos = modeName.find(' ');
        if (spacePos == std::string::npos) {
            throw GitError::GitError("Tree entry galat hai: " + modeName);
        }

        std::string mode = modeName.substr(0, spacePos);
        std::string name = modeName.substr(spacePos + 1);

        pos = nullPos + 1;
        if (pos + kRawHashLength > content.size()) {
            throw GitError::GitError("Tree vich binary SHA-1 adhoora hai");
        }

        std::string hash;
        hash.reserve(kRawHashLength * 2);
        for (size_t i = 0; i < kRawHashLength; ++i) {
            hash += hexByte(static_cast<unsigned char>(content[pos + i]));
        }

        entries.emplace_back(mode, name, hash);
        pos += kRawHashLength;
    }

    return entries;
}

std::string serializeTree(const std::vector<TreeEntry>& entries) {
    std::string out;
    for (const auto& entry : entries) {
        out += entry.mode;
        out += ' ';
        out += entry.name;
        out.push_back('\0');

        if (entry.hash.size() != 40) {
            throw GitError::GitError("Tree entry da hash 40 characters da nahi: " + entry.hash);
        }
        for (size_t i = 0; i < entry.hash.size(); i += 2) {
            unsigned char byte = static_cast<unsigned char>(
                (fromHexNibble(entry.hash[i]) << 4) | fromHexNibble(entry.hash[i + 1]));
            out.push_back(static_cast<char>(byte));
        }
    }
    return out;
}

void printTree(const std::filesystem::path& root, const std::string& hash, bool name_only) {
    GitObject::Object object = GitObject::splitObject(GitObject::readObject(root, hash));
    if (object.type != "tree") {
        throw GitError::GitError("Eh tree nahi hai: " + hash + " (type " + object.type + ")");
    }

    for (const auto& entry : parseTree(object.body)) {
        if (name_only) {
            std::cout << entry.name << "\n";
        } else {
            std::cout << entry.mode << " " << entry.typeName() << " "
                      << entry.hash << "\t" << entry.name << "\n";
        }
    }
}

std::string writeTree(const std::filesystem::path& root, const std::filesystem::path& path) {
    std::vector<TreeEntry> entries;

    // directory_iterator order di guarantee nahi, is liye had neeche sort hoga
    // (directory_iterator has no defined order, so we sort below)
    for (const auto& child : std::filesystem::directory_iterator(path)) {
        std::string name = child.path().filename().string();

        // .git nu khud na pakdein
        // (Don't catch ourselves in .git)
        if (name == ".git") {
            continue;
        }

        std::string mode;
        std::string hash;

        std::error_code ec;
        if (std::filesystem::is_directory(child.path(), ec)) {
            hash = writeTree(root, child.path());
            mode = "40000";
        } else if (std::filesystem::is_regular_file(child.path(), ec)) {
            std::ifstream file(child.path(), std::ios::binary);
            if (!file) {
                std::cerr << "Warning: " << child.path() << " nahi padh sakdi, skip\n";
                continue;
            }
            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            hash = GitObject::createBlob(root, content);

            // Executable bit check karo - owner_exec dekhna hai
            // (Check the executable bit - owner_exec is the one to look at)
            auto perms = std::filesystem::status(child.path(), ec).permissions();
            bool executable = (perms & std::filesystem::perms::owner_exec) !=
                              std::filesystem::perms::none;
            mode = executable ? "100755" : "100644";
        } else {
            // Symlink ya koi hor exotic cheez
            // (A symlink or some other exotic thing)
            continue;
        }

        entries.emplace_back(mode, name, hash);
    }

    // Git entries nu naam de mutabik sort karda hai
    // (Git sorts entries by name)
    std::sort(entries.begin(), entries.end(), [](const TreeEntry& a, const TreeEntry& b) {
        return a.name < b.name;
    });

    return GitObject::writeObject(root, "tree", serializeTree(entries));
}

} // namespace GitTree
