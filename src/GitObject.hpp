#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <openssl/sha.h>
#include "Compression.hpp"

// ==========================================
// GIT OBJECTS DA DABBA
// (The box of Git objects)
// ==========================================

namespace GitObject {

/**
 * Vekho ji, SHA-1 hash banane da function
 * (Look here, function to create SHA-1 hash)
 * 
 * Git vich har cheez di ek unique ID hundi hai - SHA-1 hash
 * Exactly aise hi jaise har punjabi da ek unique style hunda hai!
 * (In Git everything has a unique ID - SHA-1 hash
 *  Exactly like every Punjabi has a unique style!)
 * 
 * @param data - Data jiska hash banana hai (Data whose hash to create)
 * @return 40-character hex string hash
 */
inline std::string calculateSHA1(const std::string& data) {
    // SHA-1 calculator ready karo - chakki vaang!
    // (Get SHA-1 calculator ready - like a mill!)
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), 
         data.length(), hash);
    
    // Hash nu hex string vich convert karo
    // (Convert hash to hex string)
    std::stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') 
           << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

/**
 * Git object read karne da function - kitaab padhni vaang
 * (Function to read Git object - like reading a book)
 * 
 * .git/objects folder vichon file padhta hai
 * (Reads file from .git/objects folder)
 * 
 * @param hash - SHA-1 hash of the object
 * @return Decompressed object content
 */
inline std::string readObject(const std::string& hash) {
    // Path banao - pehle 2 characters folder name, baaki file name
    // (Create path - first 2 chars folder name, rest file name)
    std::string dir = ".git/objects/" + hash.substr(0, 2);
    std::string file = hash.substr(2);
    std::string path = dir + "/" + file;
    
    // Check karo ki file hai bhi ya nahi
    // (Check if file exists or not)
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Arre bapu! Object nahi mila: " + hash + 
                               "\n(Oh father! Object not found: " + hash + ")");
    }
    
    // File kholo te padho - binary mode vich
    // (Open and read file - in binary mode)
    std::ifstream file_stream(path, std::ios::binary);
    if (!file_stream) {
        throw std::runtime_error("File khol nahi sakda! (Can't open file!)");
    }
    
    // Saara data read kar lo
    // (Read all the data)
    std::vector<uint8_t> compressed(
        (std::istreambuf_iterator<char>(file_stream)),
        std::istreambuf_iterator<char>()
    );
    
    // Decompress kar ke return karo!
    // (Decompress and return!)
    return GitCompression::decompress(compressed);
}

/**
 * Git object write karne da function - diary vich likhna vaang
 * (Function to write Git object - like writing in diary)
 * 
 * Object nu compress karke .git/objects vich save karda hai
 * (Compresses object and saves it in .git/objects)
 * 
 * @param type - Object type (blob, tree, commit)
 * @param content - Object content
 * @return SHA-1 hash of the created object
 */
inline std::string writeObject(const std::string& type, const std::string& content) {
    // Git object format: "type size\0content"
    // Bilkul recipe vaang - pehle ingredients, phir directions!
    // (Just like a recipe - first ingredients, then directions!)
    std::string header = type + " " + std::to_string(content.size()) + '\0';
    std::string full_content = header + content;
    
    // SHA-1 hash calculate karo - unique ID mil jayegi
    // (Calculate SHA-1 hash - will get unique ID)
    std::string hash = calculateSHA1(full_content);
    
    // Directory path te file path banao
    // (Create directory path and file path)
    std::string dir = ".git/objects/" + hash.substr(0, 2);
    std::string file_path = dir + "/" + hash.substr(2);
    
    // Agar object pehle se hai, dobara create karn di zaroorat nahi
    // (If object already exists, no need to create again)
    if (std::filesystem::exists(file_path)) {
        return hash; // Kaam khatam! (Job done!)
    }
    
    // Directory banao agar nahi hai
    // (Create directory if it doesn't exist)
    std::filesystem::create_directories(dir);
    
    // Content compress karo - jagah bachao!
    // (Compress content - save space!)
    auto compressed = GitCompression::compress(full_content);
    
    // File vich likho - binary mode vich
    // (Write to file - in binary mode)
    std::ofstream file_stream(file_path, std::ios::binary);
    if (!file_stream) {
        throw std::runtime_error("File likh nahi sakda! (Can't write file!)");
    }
    
    file_stream.write(reinterpret_cast<const char*>(compressed.data()), 
                      compressed.size());
    
    return hash;
}

/**
 * Blob object banane da function
 * (Function to create blob object)
 * 
 * Blob = Binary Large OBject
 * Simple file content store karda hai, bass!
 * (Simply stores file content, that's it!)
 * 
 * @param content - File content
 * @return SHA-1 hash of the blob
 */
inline std::string createBlob(const std::string& content) {
    // Seedha writeObject nu bulao - easy peasy!
    // (Just call writeObject - easy peasy!)
    return writeObject("blob", content);
}

/**
 * Blob object nu console te print karne da function
 * (Function to print blob object to console)
 * 
 * Cat-file command vaang kaam karda hai
 * (Works like cat-file command)
 * 
 * @param hash - SHA-1 hash of the blob
 */
inline void printBlob(const std::string& hash) {
    // Object padho
    // (Read object)
    std::string content = readObject(hash);
    
    // Header te content alag karo - null character te split karo
    // (Separate header and content - split on null character)
    size_t null_pos = content.find('\0');
    if (null_pos == std::string::npos) {
        throw std::runtime_error("Invalid blob format - null byte nahi mila!"
                               "\n(Invalid blob format - null byte not found!)");
    }
    
    // Header check karo ki blob hai ya nahi
    // (Check header if it's a blob or not)
    std::string header = content.substr(0, null_pos);
    if (header.substr(0, 4) != "blob") {
        throw std::runtime_error("Eh blob nahi hai yaar!"
                               "\n(This ain't a blob buddy!)");
    }
    
    // Content print karo - simple!
    // (Print content - simple!)
    std::cout << content.substr(null_pos + 1);
}

/**
 * Object da type te size return karne da function
 * (Function to return object's type and size)
 * 
 * @param hash - SHA-1 hash of the object
 * @return Pair of (type, size)
 */
inline std::pair<std::string, size_t> getObjectInfo(const std::string& hash) {
    // Object read karo
    // (Read object)
    std::string content = readObject(hash);
    
    // Header parse karo
    // (Parse header)
    size_t null_pos = content.find('\0');
    if (null_pos == std::string::npos) {
        throw std::runtime_error("Invalid object format!");
    }
    
    std::string header = content.substr(0, null_pos);
    size_t space_pos = header.find(' ');
    
    std::string type = header.substr(0, space_pos);
    size_t size = std::stoull(header.substr(space_pos + 1));
    
    return {type, size};
}

} // namespace GitObject
