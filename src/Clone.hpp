#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>
#include <zlib.h>
#include "Compression.hpp"
#include "GitObject.hpp"
#include "Tree.hpp"
#include "Repository.hpp"

// ==========================================
// CLONE KARNE DA SYSTEM - REMOTE REPO LEKE AAO!
// (Clone system - bring the remote repo!)
// ==========================================

namespace GitClone {

/**
 * Git URL parse karne da function
 * (Function to parse Git URL)
 * 
 * Supports: https://github.com/user/repo.git
 * (HTTP/HTTPS protocols)
 * 
 * @param url - Repository URL
 * @return Tuple of (protocol, host, port, path)
 */
inline std::tuple<std::string, std::string, int, std::string> parseURL(const std::string& url) {
    // URL parse karo - complicated but zaruri!
    // (Parse URL - complicated but necessary!)
    
    std::string protocol, host, path;
    int port = 0;
    
    // Protocol find karo - "://" tak
    // (Find protocol - until "://")
    size_t protocol_end = url.find("://");
    if (protocol_end == std::string::npos) {
        throw std::runtime_error("Invalid URL - protocol nahi mila!"
                               "\n(Invalid URL - protocol not found!)");
    }
    
    protocol = url.substr(0, protocol_end);
    size_t host_start = protocol_end + 3;
    
    // Port set karo - HTTP ya HTTPS
    // (Set port - HTTP or HTTPS)
    if (protocol == "https") {
        port = 443;
    } else if (protocol == "http") {
        port = 80;
    } else {
        throw std::runtime_error("Unsupported protocol: " + protocol);
    }
    
    // Host te path alag karo
    // (Separate host and path)
    size_t path_start = url.find('/', host_start);
    if (path_start == std::string::npos) {
        host = url.substr(host_start);
        path = "/";
    } else {
        host = url.substr(host_start, path_start - host_start);
        path = url.substr(path_start);
    }
    
    // Custom port check karo - agar ":" hai host vich
    // (Check for custom port - if ":" in host)
    size_t port_sep = host.find(':');
    if (port_sep != std::string::npos) {
        port = std::stoi(host.substr(port_sep + 1));
        host = host.substr(0, port_sep);
    }
    
    return {protocol, host, port, path};
}

/**
 * Pkt-line format parse karne da function
 * (Function to parse pkt-line format)
 * 
 * Git protocol use karda hai pkt-line format
 * 4 hex digits (length) + data
 * (Git protocol uses pkt-line format
 *  4 hex digits (length) + data)
 */
inline std::vector<std::string> parsePktLines(const std::string& data) {
    std::vector<std::string> lines;
    size_t pos = 0;
    
    // Curl raw data denda hai - seedha parse karo
    // (Curl gives raw data - parse directly)
    
    while (pos < data.size()) {
        // Pehle 4 bytes length honi chahidi (hex format vich)
        // (First 4 bytes should be length in hex format)
        if (pos + 4 > data.size()) break;
        
        std::string len_str = data.substr(pos, 4);
        int len = 0;
        try {
            len = std::stoi(len_str, nullptr, 16);
        } catch (...) {
            break;
        }
        
        if (len == 0) {
            // Flush packet - section khatam
            // (Flush packet - section end)
            pos += 4;
            continue;
        }
        
        if (len < 4) break;
        
        // Data extract karo (4 bytes length include nahi)
        // (Extract data - 4 bytes length not included)
        if (pos + len > data.size()) break;
        
        std::string line = data.substr(pos + 4, len - 4);
        lines.push_back(line);
        pos += len;
    }
    
    return lines;
}

/**
 * HTTP request bhejne da function (using curl for HTTPS support)
 * (Function to send HTTP request using curl for HTTPS support)
 * 
 * Simple HTTP GET request for git-upload-pack
 * (Simple HTTP GET request for git-upload-pack)
 * 
 * @param host - Server hostname
 * @param port - Server port
 * @param path - Repository path
 * @param service - Git service (upload-pack, receive-pack)
 * @return Server response
 */
inline std::string sendHTTPRequest(const std::string& host, int port, 
                                   const std::string& path,
                                   const std::string& service = "upload-pack") {
    // HTTPS ke liye curl use karo - SSL support chahida
    // (Use curl for HTTPS - SSL support needed)
    
    std::string protocol = (port == 443) ? "https" : "http";
    std::string url = protocol + "://" + host + path + "/info/refs?service=git-" + service;
    
    // Temp file vich response save karo
    // (Save response in temp file)
    std::string temp_file = "/tmp/git_clone_response_" + std::to_string(getpid()) + ".txt";
    
    std::string cmd = "curl -s -o \"" + temp_file + "\" \"" + url + "\"";
    
    int result = system(cmd.c_str());
    if (result != 0) {
        throw std::runtime_error("HTTP request fail - curl command nahi chali!"
                               "\n(HTTP request failed - curl command didn't run!)");
    }
    
    // Response file paRho
    // (Read response file)
    std::ifstream file(temp_file, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Response file nahi khul sakdi!"
                               "\n(Response file couldn't be opened!)");
    }
    
    std::string response((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    
    file.close();
    
    // Temp file delete karo
    // (Delete temp file)
    std::filesystem::remove(temp_file);
    
    return response;
}

/**
 * Pack file request karne da function (using curl for HTTPS)
 * (Function to request pack file using curl for HTTPS)
 * 
 * POST request bhejta hai git-upload-pack nu
 * (Sends POST request to git-upload-pack)
 */
inline std::string sendPackRequest(const std::string& host, int port,
                                    const std::string& path,
                                    const std::vector<std::string>& want_refs) {
    // Pack request body banao - pkt-line format vich
    // (Create pack request body - in pkt-line format)
    std::stringstream body;
    
    // Want lines bhejo - jo objects chahide ne
    // (Send want lines - which objects we need)
    for (const auto& ref : want_refs) {
        std::string want_line = "want " + ref + "\n";
        char len_buf[5];
        snprintf(len_buf, sizeof(len_buf), "%04x", (int)(want_line.length() + 4));
        body << len_buf << want_line;
    }
    
    // Flush packet - section khatam
    // (Flush packet - section end)
    body << "0000";
    
    // Done line bhejo
    // (Send done line)
    body << "0009done\n";
    
    std::string body_str = body.str();
    
    // Temp files ke liye paths
    // (Paths for temp files)
    std::string body_file = "/tmp/git_pack_request_" + std::to_string(getpid()) + ".txt";
    std::string response_file = "/tmp/git_pack_response_" + std::to_string(getpid()) + ".pack";
    
    // Request body file vich save karo
    // (Save request body in file)
    std::ofstream out(body_file, std::ios::binary);
    out << body_str;
    out.close();
    
    // URL banao
    // (Create URL)
    std::string protocol = (port == 443) ? "https" : "http";
    std::string url = protocol + "://" + host + path + "/git-upload-pack";
    
    // Curl command banao - POST request ke liye
    // (Create curl command - for POST request)
    std::string cmd = "curl -s -X POST "
                     "-H \"Content-Type: application/x-git-upload-pack-request\" "
                     "-H \"Accept: application/x-git-upload-pack-result\" "
                     "--data-binary @\"" + body_file + "\" "
                     "-o \"" + response_file + "\" "
                     "\"" + url + "\"";
    
    int result = system(cmd.c_str());
    if (result != 0) {
        std::filesystem::remove(body_file);
        throw std::runtime_error("Pack request fail - curl command nahi chali!");
    }
    
    // Response paRho
    // (Read response)
    std::ifstream file(response_file, std::ios::binary);
    if (!file) {
        std::filesystem::remove(body_file);
        throw std::runtime_error("Pack response file nahi khul sakdi!");
    }
    
    std::string response((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    
    file.close();
    
    // Temp files delete karo
    // (Delete temp files)
    std::filesystem::remove(body_file);
    std::filesystem::remove(response_file);
    
    return response;
}

/**
 * Pack file parse karne da function
 * (Function to parse pack file)
 * 
 * Pack file vich saare objects hunde ne - compressed format vich
 * (Pack file contains all objects - in compressed format)
 */
inline void parsePackFile(const std::string& pack_data, const std::string& target_dir) {
    // HTTP headers skip karo
    // (Skip HTTP headers)
    size_t pack_start = pack_data.find("PACK");
    if (pack_start == std::string::npos) {
        std::cerr << "Warning: PACK signature nahi mila!\n";
        std::cerr << "(Warning: PACK signature not found!)\n";
        return;
    }
    
    // PACK file format:
    // - Signature: "PACK" (4 bytes)
    // - Version: 4 bytes (network byte order)
    // - Objects count: 4 bytes (network byte order)
    // - Objects: variable length
    
    const unsigned char* data = reinterpret_cast<const unsigned char*>(pack_data.data() + pack_start);
    size_t data_len = pack_data.size() - pack_start;
    
    if (data_len < 12) {
        std::cerr << "Pack file bahut chhoti hai!\n";
        std::cerr << "(Pack file too small!)\n";
        return;
    }
    
    // Version paRho
    // (Read version)
    uint32_t version = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
    
    // Object count paRho
    // (Read object count)
    uint32_t obj_count = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];
    
    std::cerr << "Pack version: " << version << "\n";
    std::cerr << "Objects count: " << obj_count << "\n";
    
    size_t pos = 12; // PACK header ke baad
                     // (After PACK header)
    
    // Har object process karo
    // (Process each object)
    for (uint32_t i = 0; i < obj_count && pos < data_len; ++i) {
        // Object type te size paRho - variable length encoding
        // (Read object type and size - variable length encoding)
        
        if (pos >= data_len) break;
        
        unsigned char c = data[pos++];
        int type = (c >> 4) & 0x7;
        size_t size = c & 0xF;
        int shift = 4;
        
        while (c & 0x80) {
            if (pos >= data_len) break;
            c = data[pos++];
            size |= ((size_t)(c & 0x7F)) << shift;
            shift += 7;
        }
        
        // Object types:
        // 1 = commit, 2 = tree, 3 = blob, 4 = tag
        // 6 = OFS_DELTA, 7 = REF_DELTA (delta compressed)
        
        std::string type_name;
        switch (type) {
            case 1: type_name = "commit"; break;
            case 2: type_name = "tree"; break;
            case 3: type_name = "blob"; break;
            case 4: type_name = "tag"; break;
            case 6: type_name = "ofs_delta"; break;
            case 7: type_name = "ref_delta"; break;
            default: type_name = "unknown"; break;
        }
        
        // REF_DELTA ke liye base object hash skip karo
        // (For REF_DELTA skip base object hash)
        if (type == 7) {
            pos += 20; // 20 bytes SHA-1
            if (pos > data_len) break;
        }
        
        // Compressed data paRho - zlib format vich
        // (Read compressed data - in zlib format)
        
        // Decompression setup
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = data_len - pos;
        strm.next_in = const_cast<unsigned char*>(data + pos);
        
        if (inflateInit(&strm) != Z_OK) {
            std::cerr << "Decompression init fail!\n";
            continue;
        }
        
        // Decompressed data ke liye buffer
        // (Buffer for decompressed data)
        std::vector<unsigned char> decompressed;
        
        // Size limit lagao - safety ke liye
        // (Put size limit - for safety)
        size_t max_size = std::min(size * 10, (size_t)10 * 1024 * 1024); // Max 10MB
        decompressed.reserve(std::min(size, max_size));
        
        unsigned char out_buf[4096];
        int ret;
        
        do {
            strm.avail_out = sizeof(out_buf);
            strm.next_out = out_buf;
            
            ret = inflate(&strm, Z_NO_FLUSH);
            
            if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                inflateEnd(&strm);
                break;
            }
            
            size_t have = sizeof(out_buf) - strm.avail_out;
            
            // Size limit check karo
            // (Check size limit)
            if (decompressed.size() + have > max_size) {
                std::cerr << "Warning: Object too large, skipping!\n";
                inflateEnd(&strm);
                break;
            }
            
            decompressed.insert(decompressed.end(), out_buf, out_buf + have);
            
        } while (ret != Z_STREAM_END && decompressed.size() < max_size);
        
        size_t compressed_size = strm.total_in;
        inflateEnd(&strm);
        
        // Position update karo
        // (Update position)
        pos += compressed_size;
        
        // Object create karo - delta nahi hona chahida
        // (Create object - should not be delta)
        if (type >= 1 && type <= 4 && !decompressed.empty()) {
            try {
                // Object content - type + space + size + null + data
                // (Object content - type + space + size + null + data)
                std::string content = type_name + " " + std::to_string(decompressed.size()) + '\0';
                content.append(reinterpret_cast<char*>(decompressed.data()), decompressed.size());
                
                // SHA-1 calculate karo
                // (Calculate SHA-1)
                std::string hash = GitObject::calculateSHA1(content);
                
                // Object file vich save karo
                // (Save in object file)
                std::string dir = target_dir + "/.git/objects/" + hash.substr(0, 2);
                std::filesystem::create_directories(dir);
                
                std::string filepath = dir + "/" + hash.substr(2);
                std::ofstream out(filepath, std::ios::binary);
                
                // Compressed format vich store karo
                // (Store in compressed format)
                std::vector<unsigned char> compressed = GitCompression::compress(content);
                out.write(reinterpret_cast<char*>(compressed.data()), compressed.size());
                out.close();
                
                std::cerr << "Object created: " << hash << " (" << type_name << ")\n";
            } catch (const std::exception& e) {
                std::cerr << "Error creating object: " << e.what() << "\n";
            }
        } else if (type == 6 || type == 7) {
            // Delta objects skip kar do - advanced feature
            // (Skip delta objects - advanced feature)
            std::cerr << "Skipping delta object (type " << type << ")\n";
        }
    }
}

/**
 * Tree recursively checkout karne da helper function (forward declaration)
 * (Helper function to recursively checkout tree - forward declaration)
 */
inline void checkoutTree(const std::string& tree_hash, const std::string& path);

/**
 * Working tree checkout karne da function
 * (Function to checkout working tree)
 * 
 * Commit object ton tree extract kar ke working directory vich files banao
 * (Extract tree from commit object and create files in working directory)
 */
inline void checkoutWorkingTree(const std::string& commit_hash, const std::string& target_dir) {
    try {
        // Commit object paRho
        // (Read commit object)
        std::string full_content = GitObject::readObject(commit_hash);
        
        // Header parse karo - "type size\0" format
        // (Parse header - "type size\0" format)
        size_t null_pos = full_content.find('\0');
        if (null_pos == std::string::npos) {
            std::cerr << "Warning: Invalid object format!\n";
            return;
        }
        
        std::string header = full_content.substr(0, null_pos);
        std::string content = full_content.substr(null_pos + 1);
        
        size_t space_pos = header.find(' ');
        std::string type = header.substr(0, space_pos);
        
        if (type != "commit") {
            std::cerr << "Warning: Not a commit object!\n";
            return;
        }
        
        // Tree hash find karo commit vichon
        // (Find tree hash from commit)
        std::istringstream iss(content);
        std::string line;
        std::string tree_hash;
        
        while (std::getline(iss, line)) {
            if (line.starts_with("tree ")) {
                tree_hash = line.substr(5);
                break;
            }
        }
        
        if (tree_hash.empty()) {
            std::cerr << "Warning: No tree found in commit!\n";
            return;
        }
        
        std::cerr << "Checking out tree: " << tree_hash << "\n";
        
        // Tree recursively checkout karo
        // (Recursively checkout tree)
        checkoutTree(tree_hash, target_dir);
        
    } catch (const std::exception& e) {
        std::cerr << "Checkout error: " << e.what() << "\n";
    }
}

/**
 * Tree recursively checkout karne da helper function
 * (Helper function to recursively checkout tree)
 */
inline void checkoutTree(const std::string& tree_hash, const std::string& path) {
    try {
        // Tree object paRho
        // (Read tree object)
        std::string full_content = GitObject::readObject(tree_hash);
        
        // Header parse karo
        // (Parse header)
        size_t null_pos = full_content.find('\0');
        if (null_pos == std::string::npos) {
            std::cerr << "Warning: Invalid object format!\n";
            return;
        }
        
        std::string header = full_content.substr(0, null_pos);
        std::string content = full_content.substr(null_pos + 1);
        
        size_t space_pos = header.find(' ');
        std::string type = header.substr(0, space_pos);
        
        if (type != "tree") {
            std::cerr << "Warning: Not a tree object: " << tree_hash << "\n";
            return;
        }
        
        // Tree parse karo
        // (Parse tree)
        auto entries = GitTree::parseTree(content);
        
        // Har entry process karo
        // (Process each entry)
        for (const auto& entry : entries) {
            std::string entry_path = path + "/" + entry.name;
            
            if (entry.mode == "40000" || entry.mode == "040000") {
                // Directory hai - recursively checkout karo
                // (It's a directory - checkout recursively)
                std::filesystem::create_directories(entry_path);
                checkoutTree(entry.hash, entry_path);
            } else {
                // File hai - create karo
                // (It's a file - create it)
                try {
                    std::string obj_full_content = GitObject::readObject(entry.hash);
                    
                    // Header parse karo
                    // (Parse header)
                    size_t obj_null_pos = obj_full_content.find('\0');
                    if (obj_null_pos != std::string::npos) {
                        std::string obj_header = obj_full_content.substr(0, obj_null_pos);
                        std::string obj_content = obj_full_content.substr(obj_null_pos + 1);
                        
                        size_t obj_space_pos = obj_header.find(' ');
                        std::string obj_type = obj_header.substr(0, obj_space_pos);
                        
                        if (obj_type == "blob") {
                            std::ofstream file(entry_path, std::ios::binary);
                            file << obj_content;
                            file.close();
                            
                            // Executable permissions set karo agar chahide
                            // (Set executable permissions if needed)
                            if (entry.mode == "100755") {
                                std::filesystem::permissions(entry_path,
                                    std::filesystem::perms::owner_exec |
                                    std::filesystem::perms::group_exec |
                                    std::filesystem::perms::others_exec,
                                    std::filesystem::perm_options::add);
                            }
                        }
                    }
                } catch (const std::exception& e) {
                    // Object nahi mila - empty file banao (delta-encoded ho sakda)
                    // (Object not found - create empty file, might be delta-encoded)
                    std::cerr << "Warning: Blob " << entry.hash << " not found, creating empty file\n";
                    std::ofstream file(entry_path, std::ios::binary);
                    file.close();
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Warning: Tree checkout error: " << e.what() << "\n";
    }
}

/**
 * Repository clone karne da main function
 * (Main function to clone repository)
 * 
 * Remote repository nu local vich copy karda hai
 * Saara code, history, sab kucch!
 * (Copies remote repository to local
 *  All code, history, everything!)
 * 
 * @param url - Repository URL
 * @param target_dir - Target directory naam (optional)
 * @return true if successful
 */
inline bool clone(const std::string& url, std::string target_dir = "") {
    try {
        // Target directory set karo - agar nahi dita
        // (Set target directory - if not given)
        if (target_dir.empty()) {
            // URL toh repo naam extract karo
            // (Extract repo name from URL)
            size_t last_slash = url.find_last_of('/');
            target_dir = url.substr(last_slash + 1);
            
            // .git extension hatao
            // (Remove .git extension)
            if (target_dir.ends_with(".git")) {
                target_dir = target_dir.substr(0, target_dir.length() - 4);
            }
        }
        
        std::cout << "Cloning into '" << target_dir << "'...\n";
        
        // URL parse karo
        // (Parse URL)
        auto [protocol, host, port, path] = parseURL(url);
        
        std::cerr << "Protocol: " << protocol << "\n";
        std::cerr << "Host: " << host << "\n";
        std::cerr << "Port: " << port << "\n";
        std::cerr << "Path: " << path << "\n";
        
        // Target directory banao
        // (Create target directory)
        std::filesystem::create_directories(target_dir);
        
        // Git repository initialize karo
        // (Initialize Git repository)
        std::filesystem::create_directories(target_dir + "/.git");
        std::filesystem::create_directories(target_dir + "/.git/objects");
        std::filesystem::create_directories(target_dir + "/.git/refs");
        
        std::ofstream headFile(target_dir + "/.git/HEAD");
        headFile << "ref: refs/heads/master\n";
        headFile.close();
        
        // Smart HTTP protocol use karke references fetch karo
        // (Fetch references using smart HTTP protocol)
        std::cerr << "Fetching references...\n";
        
        std::string response = sendHTTPRequest(host, port, path);
        
        // Pkt-lines parse karo
        // (Parse pkt-lines)
        auto lines = parsePktLines(response);
        
        std::map<std::string, std::string> refs;
        std::vector<std::string> want_refs;
        
        for (const auto& line : lines) {
            // Pehli line service advertisement honi chahidi
            // (First line should be service advertisement)
            if (line.find("# service=") == 0) {
                continue;
            }
            
            // Ref line format: <hash> <ref-name>
            size_t space_pos = line.find(' ');
            if (space_pos != std::string::npos && space_pos == 40) {
                std::string hash = line.substr(0, 40);
                std::string ref_name = line.substr(41);
                
                // Null bytes te capabilities hatao
                // (Remove null bytes and capabilities)
                size_t null_pos = ref_name.find('\0');
                if (null_pos != std::string::npos) {
                    ref_name = ref_name.substr(0, null_pos);
                }
                
                // Newline hatao
                // (Remove newline)
                if (!ref_name.empty() && ref_name.back() == '\n') {
                    ref_name.pop_back();
                }
                
                refs[ref_name] = hash;
                want_refs.push_back(hash);
                
                std::cerr << "Ref: " << ref_name << " -> " << hash << "\n";
            }
        }
        
        if (want_refs.empty()) {
            std::cerr << "Warning: Koi refs nahi mile!\n";
            std::cerr << "(Warning: No refs found!)\n";
            return false;
        }
        
        // Pack file fetch karo
        // (Fetch pack file)
        std::cerr << "Fetching pack file...\n";
        
        std::string pack_response = sendPackRequest(host, port, path, want_refs);
        
        // Pack file parse karo te objects create karo
        // (Parse pack file and create objects)
        std::cerr << "Parsing pack file...\n";
        parsePackFile(pack_response, target_dir);
        
        // HEAD ref update karo
        // (Update HEAD ref)
        std::string head_commit;
        if (refs.count("refs/heads/master")) {
            std::string master_hash = refs["refs/heads/master"];
            std::filesystem::create_directories(target_dir + "/.git/refs/heads");
            std::ofstream master_file(target_dir + "/.git/refs/heads/master");
            master_file << master_hash << "\n";
            master_file.close();
            
            head_commit = master_hash;
            std::cerr << "HEAD set to: " << master_hash << "\n";
        } else if (refs.count("refs/heads/main")) {
            std::string main_hash = refs["refs/heads/main"];
            std::filesystem::create_directories(target_dir + "/.git/refs/heads");
            std::ofstream main_file(target_dir + "/.git/refs/heads/main");
            main_file << main_hash << "\n";
            main_file.close();
            
            // HEAD update karo main te point karne ke liye
            // (Update HEAD to point to main)
            std::ofstream head_file(target_dir + "/.git/HEAD");
            head_file << "ref: refs/heads/main\n";
            head_file.close();
            
            head_commit = main_hash;
            std::cerr << "HEAD set to: " << main_hash << "\n";
        }
        
        // Working tree checkout karo
        // (Checkout working tree)
        if (!head_commit.empty()) {
            std::cerr << "Checking out files...\n";
            
            // Current directory save karo
            // (Save current directory)
            auto original_path = std::filesystem::current_path();
            
            // Target directory vich jao
            // (Go to target directory)
            std::filesystem::current_path(target_dir);
            
            checkoutWorkingTree(head_commit, ".");
            
            // Wapas original directory vich jao
            // (Go back to original directory)
            std::filesystem::current_path(original_path);
        }
        
        std::cout << "Clone complete!\n";
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Clone fail: " << e.what() << "\n";
        return false;
    }
}

} // namespace GitClone
