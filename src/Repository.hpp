#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

// ==========================================
// REPOSITORY DA GHAR BANANA
// (Building the repository's home)
// ==========================================

namespace GitRepository {

/**
 * Git repository initialize karne da function
 * (Function to initialize Git repository)
 * 
 * Eh .git folder banata hai saare zaruri folders te files ke naal
 * Bilkul ghar banana - pehle neev rakho, phir kamre banao!
 * (This creates .git folder with all necessary folders and files
 *  Just like building a house - first lay foundation, then make rooms!)
 * 
 * @return true if successful, false otherwise
 */
inline bool init() {
    try {
        // Main .git directory - repository da dil!
        // (Main .git directory - heart of repository!)
        std::filesystem::create_directory(".git");
        
        // Objects directory - sab kucch yahan store hunda hai
        // (Objects directory - everything gets stored here)
        std::filesystem::create_directory(".git/objects");
        
        // Refs directory - branches te tags ke liye
        // (Refs directory - for branches and tags)
        std::filesystem::create_directory(".git/refs");
        std::filesystem::create_directory(".git/refs/heads");
        std::filesystem::create_directory(".git/refs/tags");
        
        // HEAD file - current branch da pointer
        // (HEAD file - pointer to current branch)
        std::ofstream headFile(".git/HEAD");
        if (headFile.is_open()) {
            // Default branch "main" set karo - modern Git vaang!
            // (Set default branch to "main" - like modern Git!)
            headFile << "ref: refs/heads/main\n";
            headFile.close();
        } else {
            throw std::runtime_error("HEAD file nahi ban sakdi!"
                                   "\n(HEAD file couldn't be created!)");
        }
        
        // Config file banao - settings te preferences ke liye
        // (Create config file - for settings and preferences)
        std::ofstream configFile(".git/config");
        if (configFile.is_open()) {
            configFile << "[core]\n";
            configFile << "\trepositoryformatversion = 0\n";
            configFile << "\tfilemode = true\n";
            configFile << "\tbare = false\n";
            configFile << "\tlogallrefupdates = true\n";
            configFile.close();
        }
        
        // Description file - repository da description
        // (Description file - repository's description)
        std::ofstream descFile(".git/description");
        if (descFile.is_open()) {
            descFile << "Unnamed repository; edit this file 'description' to name the repository.\n";
            descFile.close();
        }
        
        std::cout << "Initialized git directory\n";
        return true;
        
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Arre bapu! Repository init vich problem: " << e.what() 
                  << "\n(Oh father! Problem in repository init: " << e.what() << ")\n";
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Koi masla aa gaya: " << e.what() 
                  << "\n(Some problem occurred: " << e.what() << ")\n";
        return false;
    }
}

/**
 * Check karo ki current directory ek Git repository hai ya nahi
 * (Check if current directory is a Git repository or not)
 * 
 * @return true if .git directory exists
 */
inline bool isGitRepository() {
    // Simple check - .git folder hai ya nahi?
    // (Simple check - does .git folder exist?)
    return std::filesystem::exists(".git") && 
           std::filesystem::is_directory(".git");
}

/**
 * Git directory da path return karo
 * (Return path of Git directory)
 * 
 * @return Path to .git directory
 */
inline std::string getGitDir() {
    if (!isGitRepository()) {
        throw std::runtime_error("Eh Git repository nahi hai!"
                               "\n(This is not a Git repository!)");
    }
    return ".git";
}

/**
 * Objects directory da path return karo
 * (Return path of objects directory)
 * 
 * @return Path to .git/objects directory
 */
inline std::string getObjectsDir() {
    return getGitDir() + "/objects";
}

/**
 * Refs directory da path return karo
 * (Return path of refs directory)
 * 
 * @return Path to .git/refs directory
 */
inline std::string getRefsDir() {
    return getGitDir() + "/refs";
}

/**
 * HEAD file da content read karo
 * (Read content of HEAD file)
 * 
 * @return Content of HEAD file
 */
inline std::string readHEAD() {
    std::string headPath = getGitDir() + "/HEAD";
    std::ifstream headFile(headPath);
    
    if (!headFile) {
        throw std::runtime_error("HEAD file nahi mil sakdi!"
                               "\n(HEAD file not found!)");
    }
    
    std::string content;
    std::getline(headFile, content);
    return content;
}

/**
 * Current branch da naam return karo
 * (Return name of current branch)
 * 
 * @return Branch name
 */
inline std::string getCurrentBranch() {
    std::string head = readHEAD();
    
    // HEAD format: "ref: refs/heads/main"
    // Toh "refs/heads/" ke baad jo hai, woh branch name hai
    // (So whatever is after "refs/heads/" is the branch name)
    if (head.find("ref: refs/heads/") == 0) {
        return head.substr(16); // "ref: refs/heads/" = 16 characters
    }
    
    // Agar detached HEAD hai, toh hash return karo
    // (If detached HEAD, return hash)
    return head;
}

} // namespace GitRepository
