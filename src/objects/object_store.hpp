#pragma once

// ==========================================
// GIT OBJECTS DA DABBA
// (The box of Git objects)
// ==========================================

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace GitObject {

/**
 * Koi bhi Git object da eh chehra
 * (The face of any Git object)
 *
 * Raw object file vich "<type> <size>\0<body>" hunda hai
 * (A raw loose object file is "<type> <size>\0<body>")
 */
struct Object {
    std::string type;   // blob | tree | commit | tag
    std::string body;   // header ke baad da poora content

    // Size header toh nikal lo, warna har jagah parse karna painda
    // (Pull the size out of the header so nobody has to parse it again)
    size_t size() const { return body.size(); }
};

/**
 * Raw bytes toh SHA-1, 40-character hex string
 * (SHA-1 of raw bytes, as a 40-character hex string)
 */
std::string calculateSHA1(const std::string& data);

/**
 * Object file da path - ".git/objects/ab/cdef..."
 * (Path of the loose object file)
 */
std::filesystem::path objectPath(const std::filesystem::path& root, const std::string& hash);

/**
 * Object maujood hai ya nahi
 * (Whether the object exists)
 */
bool hasObject(const std::filesystem::path& root, const std::string& hash);

/**
 * Object padh ke usda "<type> <size>\0<body>" return karo
 * (Read an object and return its "<type> <size>\0<body>")
 */
std::string readObject(const std::filesystem::path& root, const std::string& hash);

/**
 * Object likh ke usda hash return karo
 * (Write an object and return its hash)
 *
 * Header badhe size toh yahon body da size hi hoy - pack reader vich
 * aam objects da size pack header toh aunda hai, body toh nahi
 * (The header is built from body.size() - the pack reader knows an
 *  object's size from the pack header, not from the body it inflated)
 */
std::string writeObject(const std::filesystem::path& root,
                        const std::string& type,
                        const std::string& body);

/**
 * Blob bana ke usda hash return karo
 * (Create a blob and return its hash)
 */
std::string createBlob(const std::filesystem::path& root, const std::string& content);

/**
 * Object da type te size
 * (An object's type and size)
 */
std::pair<std::string, size_t> getObjectInfo(const std::filesystem::path& root,
                                             const std::string& hash);

/**
 * Raw "<type> <size>\0<body>" nu Object vich kholo
 * (Split a raw "<type> <size>\0<body>" into an Object)
 *
 * @throws GitError agar raw string well formed nahi hai
 */
Object splitObject(const std::string& raw);

/**
 * Blob da content console te likho - binary safe, koi extra byte nahi
 * (Print a blob's content - binary safe, no extra bytes)
 */
void printBlob(const std::filesystem::path& root, const std::string& hash);

} // namespace GitObject
