#include "objects/object_store.hpp"

#include "objects/compression.hpp"
#include "repository/repository.hpp"
#include "utils/error.hpp"

#include <array>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <openssl/sha.h>

namespace GitObject {

namespace {

// Git har SHA-1 nu 40 hex characters vich likhda hai
// (Git writes every SHA-1 as 40 hex characters)
constexpr size_t kHexLength = 40;

std::string bytesToHex(const unsigned char* raw, size_t length) {
    static const char* digits = "0123456789abcdef";
    std::string hex;
    hex.reserve(length * 2);
    for (size_t i = 0; i < length; ++i) {
        hex.push_back(digits[raw[i] >> 4]);
        hex.push_back(digits[raw[i] & 0x0F]);
    }
    return hex;
}

} // namespace

std::string calculateSHA1(const std::string& data) {
    std::array<unsigned char, SHA_DIGEST_LENGTH> digest{};
    // OpenSSL 3.0 deprecates the one-shot SHA1() but it still works, and
    // SHA1() is the only API guaranteed present across every 1.1/3.x build
    if (SHA1(reinterpret_cast<const unsigned char*>(data.data()),
             data.length(),
             digest.data()) == nullptr) {
        throw GitError::GitError("SHA-1 calculate nahi hui!");
    }
    return bytesToHex(digest.data(), digest.size());
}

std::filesystem::path objectPath(const std::filesystem::path& root, const std::string& hash) {
    if (hash.size() < kHexLength) {
        throw GitError::GitError("Hash taqdeer naal short hai: " + hash);
    }
    return GitRepository::getObjectsDir(root) / hash.substr(0, 2) / hash.substr(2, kHexLength - 2);
}

bool hasObject(const std::filesystem::path& root, const std::string& hash) {
    std::error_code ec;
    return std::filesystem::exists(objectPath(root, hash), ec);
}

std::string readObject(const std::filesystem::path& root, const std::string& hash) {
    std::filesystem::path path = objectPath(root, hash);

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw GitError::GitError("Object nahi mila: " + hash);
    }

    std::vector<uint8_t> compressed((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
    return GitCompression::decompress(compressed);
}

std::string writeObject(const std::filesystem::path& root,
                        const std::string& type,
                        const std::string& body) {
    // Git loose object layout: "<type> <size>\0<body>"
    std::string full = type + " " + std::to_string(body.size());
    full.push_back('\0');
    full += body;

    std::string hash = calculateSHA1(full);
    std::filesystem::path path = objectPath(root, hash);

    // Object pehlan toh bana hove te koi kaam nahi - Git vich bhi aisa hi hai
    // (Object already there? Nothing to do - and neither does real Git)
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        return hash;
    }

    std::filesystem::create_directories(path.parent_path());

    std::vector<uint8_t> compressed = GitCompression::compress(full);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw GitError::GitError("Object likh nahi sakdi: " + path.string());
    }
    out.write(reinterpret_cast<const char*>(compressed.data()),
              static_cast<std::streamsize>(compressed.size()));
    if (!out) {
        throw GitError::GitError("Object likh nahi sakdi: " + path.string());
    }
    return hash;
}

std::string createBlob(const std::filesystem::path& root, const std::string& content) {
    return writeObject(root, "blob", content);
}

Object splitObject(const std::string& raw) {
    size_t nullPos = raw.find('\0');
    if (nullPos == std::string::npos) {
        throw GitError::GitError("Object format galat hai - null byte nahi mila!");
    }

    std::string header = raw.substr(0, nullPos);
    size_t spacePos = header.find(' ');
    if (spacePos == std::string::npos) {
        throw GitError::GitError("Object header galat hai: " + header);
    }

    Object object;
    object.type = header.substr(0, spacePos);
    object.body = raw.substr(nullPos + 1);
    return object;
}

std::pair<std::string, size_t> getObjectInfo(const std::filesystem::path& root,
                                             const std::string& hash) {
    Object object = splitObject(readObject(root, hash));
    return {object.type, object.body.size()};
}

void printBlob(const std::filesystem::path& root, const std::string& hash) {
    Object object = splitObject(readObject(root, hash));
    if (object.type != "blob") {
        throw GitError::GitError("Eh blob nahi hai: " + hash + " (type " + object.type + ")");
    }
    // write() nahi << operator - blob binary ho sakda hai
    // (write(), not operator<< - a blob can be binary)
    std::cout.write(object.body.data(), static_cast<std::streamsize>(object.body.size()));
    std::cout.flush();
}

} // namespace GitObject
