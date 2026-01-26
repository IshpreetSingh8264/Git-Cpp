#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include "GitObject.hpp"

// ==========================================
// TREE OBJECTS DA KHAJANA
// (The treasure of tree objects)
// ==========================================

namespace GitTree {

/**
 * Tree entry - har file ya directory ki jaankari
 * (Tree entry - information of each file or directory)
 */
struct TreeEntry {
    std::string mode;      // File permissions - 100644, 040000, etc.
    std::string name;      // File/directory naam (name)
    std::string hash;      // SHA-1 hash
    
    // Constructor - shuru karo party!
    // (Constructor - start the party!)
    TreeEntry(const std::string& m, const std::string& n, const std::string& h)
        : mode(m), name(n), hash(h) {}
};

/**
 * Tree object parse karne da function
 * (Function to parse tree object)
 * 
 * Tree object vich har entry aise format vich hundi hai:
 * "mode name\0" followed by 20-byte binary SHA-1
 * Thoda complicated hai, par chalda hai!
 * (Each entry in tree object is in this format:
 *  "mode name\0" followed by 20-byte binary SHA-1
 *  A bit complicated, but it works!)
 * 
 * @param content - Tree object content (without header)
 * @return Vector of TreeEntry objects
 */
inline std::vector<TreeEntry> parseTree(const std::string& content) {
    std::vector<TreeEntry> entries;
    size_t pos = 0;
    
    // Saare entries parse karo - ek ek karke
    // (Parse all entries - one by one)
    while (pos < content.size()) {
        // Mode te name padho - null byte tak
        // (Read mode and name - until null byte)
        size_t null_pos = content.find('\0', pos);
        if (null_pos == std::string::npos) {
            break; // Khatam! (Done!)
        }
        
        std::string mode_name = content.substr(pos, null_pos - pos);
        
        // Mode te name nu alag karo - space te split
        // (Separate mode and name - split on space)
        size_t space_pos = mode_name.find(' ');
        if (space_pos == std::string::npos) {
            throw std::runtime_error("Invalid tree entry format - space nahi mila!"
                                   "\n(Invalid tree entry format - space not found!)");
        }
        
        std::string mode = mode_name.substr(0, space_pos);
        std::string name = mode_name.substr(space_pos + 1);
        
        // 20-byte binary SHA-1 hash padho
        // (Read 20-byte binary SHA-1 hash)
        pos = null_pos + 1;
        if (pos + 20 > content.size()) {
            throw std::runtime_error("Invalid tree - SHA hash incomplete hai!"
                                   "\n(Invalid tree - SHA hash is incomplete!)");
        }
        
        // Binary hash nu hex string vich convert karo
        // (Convert binary hash to hex string)
        std::stringstream ss;
        for (int i = 0; i < 20; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0')
               << (static_cast<int>(content[pos + i]) & 0xFF);
        }
        std::string hash = ss.str();
        
        // Entry add karo list vich
        // (Add entry to list)
        entries.emplace_back(mode, name, hash);
        
        pos += 20; // Agle entry te jao! (Go to next entry!)
    }
    
    return entries;
}

/**
 * Tree object print karne da function - ls-tree vaang
 * (Function to print tree object - like ls-tree)
 * 
 * @param hash - Tree object da SHA-1 hash
 * @param name_only - Sirf naam print karo? (Print only names?)
 */
inline void printTree(const std::string& hash, bool name_only = false) {
    // Tree object padho
    // (Read tree object)
    std::string content = GitObject::readObject(hash);
    
    // Header te content alag karo
    // (Separate header and content)
    size_t null_pos = content.find('\0');
    if (null_pos == std::string::npos) {
        throw std::runtime_error("Invalid tree format!");
    }
    
    std::string header = content.substr(0, null_pos);
    if (header.substr(0, 4) != "tree") {
        throw std::runtime_error("Eh tree nahi hai bhai!"
                               "\n(This ain't a tree bro!)");
    }
    
    // Tree parse karo
    // (Parse tree)
    std::string tree_content = content.substr(null_pos + 1);
    auto entries = parseTree(tree_content);
    
    // Entries print karo
    // (Print entries)
    for (const auto& entry : entries) {
        if (name_only) {
            // Sirf naam - simple!
            // (Just name - simple!)
            std::cout << entry.name << "\n";
        } else {
            // Full details - mode, type, hash, name
            // Bilkul detailed report vaang!
            // (Full details - mode, type, hash, name
            //  Just like a detailed report!)
            std::string type = (entry.mode == "040000") ? "tree" : "blob";
            std::cout << entry.mode << " " << type << " " 
                      << entry.hash << "\t" << entry.name << "\n";
        }
    }
}

/**
 * Directory toh tree object banane da function
 * (Function to create tree object from directory)
 * 
 * Recursively saari files te subdirectories nu process karda hai
 * (Recursively processes all files and subdirectories)
 * 
 * @param path - Directory path
 * @return SHA-1 hash of created tree object
 */
inline std::string writeTree(const std::string& path = ".") {
    std::vector<TreeEntry> entries;
    
    // Directory vich saari files te folders scan karo
    // (Scan all files and folders in directory)
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        std::string name = entry.path().filename().string();
        
        // .git folder ignore karo - khud nu nahi pakadna!
        // (Ignore .git folder - don't catch yourself!)
        if (name == ".git") {
            continue;
        }
        
        std::string hash;
        std::string mode;
        
        if (entry.is_directory()) {
            // Subdirectory hai - recursively tree banao!
            // (It's a subdirectory - recursively create tree!)
            hash = writeTree(entry.path().string());
            mode = "40000"; // Directory mode
            
        } else if (entry.is_regular_file()) {
            // Regular file hai - blob banao!
            // (It's a regular file - create blob!)
            
            // File content padho
            // (Read file content)
            std::ifstream file(entry.path(), std::ios::binary);
            if (!file) {
                std::cerr << "Warning: " << entry.path() << " nahi padh sakda!"
                          << "\n(Warning: Can't read " << entry.path() << "!)\n";
                continue;
            }
            
            std::string content(
                (std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>()
            );
            
            // Blob create karo
            // (Create blob)
            hash = GitObject::createBlob(content);
            
            // File permissions check karo - executable hai ya nahi?
            // (Check file permissions - is it executable?)
            auto perms = std::filesystem::status(entry.path()).permissions();
            bool is_executable = (perms & std::filesystem::perms::owner_exec) != 
                               std::filesystem::perms::none;
            
            mode = is_executable ? "100755" : "100644";
            
        } else {
            // Symlink ya koi hor exotic cheez - skip karo
            // (Symlink or some other exotic thing - skip it)
            continue;
        }
        
        // Entry add karo
        // (Add entry)
        entries.emplace_back(mode, name, hash);
    }
    
    // Entries nu naam ke mutabik sort karo - Git da rule!
    // (Sort entries by name - Git's rule!)
    std::sort(entries.begin(), entries.end(),
              [](const TreeEntry& a, const TreeEntry& b) {
                  return a.name < b.name;
              });
    
    // Tree content banao - binary format vich
    // (Create tree content - in binary format)
    std::string tree_content;
    for (const auto& entry : entries) {
        // "mode name\0" + 20-byte binary hash
        tree_content += entry.mode + " " + entry.name + '\0';
        
        // Hex hash nu binary vich convert karo
        // (Convert hex hash to binary)
        for (size_t i = 0; i < entry.hash.length(); i += 2) {
            std::string byte_str = entry.hash.substr(i, 2);
            char byte = static_cast<char>(std::stoi(byte_str, nullptr, 16));
            tree_content += byte;
        }
    }
    
    // Tree object write karo te hash return karo
    // (Write tree object and return hash)
    return GitObject::writeObject("tree", tree_content);
}

} // namespace GitTree
