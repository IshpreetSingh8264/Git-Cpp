// ==========================================
// PUNJABI GIT - ASLI SWAG NAAL!
// (Punjabi Git - With real swag!)
// ==========================================
// Full Git implementation in C++ with humorous Pinglish comments
// Built for CodeCrafters Git challenge
// Modules: Repository, Objects (Blob/Tree/Commit), Clone
// ==========================================

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

// Saare apne modules import karo - team ready!
// (Import all our modules - team ready!)
#include "Repository.hpp"
#include "GitObject.hpp"
#include "Tree.hpp"
#include "Commit.hpp"
#include "Clone.hpp"

// ==========================================
// COMMAND HANDLERS - HER COMMAND DA APNA KAAM
// (Command handlers - each command has its own job)
// ==========================================

/**
 * Init command - nayi repository bana!
 * (Init command - create new repository!)
 */
void handleInit() {
    // Repository.hpp vich banaya function use karo
    // (Use function created in Repository.hpp)
    if (GitRepository::init()) {
        // Success! Wadhaiya ji!
        // (Success! Congratulations!)
    } else {
        // Fail ho gaya - error already print ho gaya
        // (Failed - error already printed)
        std::exit(EXIT_FAILURE);
    }
}

/**
 * Cat-file command - object di content dikha!
 * (Cat-file command - show object's content!)
 */
void handleCatFile(int argc, char* argv[]) {
    // Usage: cat-file -p <hash>
    // -p flag = pretty print
    
    if (argc < 4) {
        std::cerr << "Usage: cat-file -p <object-hash>\n";
        std::cerr << "Arre yaar, proper command toh use karo!"
                  << "\n(Come on buddy, use proper command!)\n";
        std::exit(EXIT_FAILURE);
    }
    
    std::string flag = argv[2];
    std::string hash = argv[3];
    
    // Sirf -p flag support karde haan abhi
    // (Only supporting -p flag for now)
    if (flag != "-p") {
        std::cerr << "Only -p flag supported yaar!"
                  << "\n(Only -p flag supported buddy!)\n";
        std::exit(EXIT_FAILURE);
    }
    
    try {
        // Object type check karo - blob, tree, ya commit?
        // (Check object type - blob, tree, or commit?)
        auto [type, size] = GitObject::getObjectInfo(hash);
        
        if (type == "blob") {
            // Blob print karo
            // (Print blob)
            GitObject::printBlob(hash);
        } else if (type == "tree") {
            // Tree print karo
            // (Print tree)
            GitTree::printTree(hash, false);
        } else if (type == "commit") {
            // Commit print karo
            // (Print commit)
            GitCommit::printCommit(hash);
        } else {
            std::cerr << "Unknown object type: " << type << "\n";
            std::exit(EXIT_FAILURE);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::exit(EXIT_FAILURE);
    }
}

/**
 * Hash-object command - file toh blob bana!
 * (Hash-object command - create blob from file!)
 */
void handleHashObject(int argc, char* argv[]) {
    // Usage: hash-object -w <file>
    // -w flag = write object to database
    
    if (argc < 4) {
        std::cerr << "Usage: hash-object -w <file>\n";
        std::exit(EXIT_FAILURE);
    }
    
    std::string flag = argv[2];
    std::string filename = argv[3];
    
    if (flag != "-w") {
        std::cerr << "Only -w flag supported!\n";
        std::exit(EXIT_FAILURE);
    }
    
    try {
        // File content padho
        // (Read file content)
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("File nahi khul sakdi: " + filename +
                                   "\n(File couldn't be opened: " + filename + ")");
        }
        
        std::string content(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );
        
        // Blob create karo te hash print karo
        // (Create blob and print hash)
        std::string hash = GitObject::createBlob(content);
        std::cout << hash << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::exit(EXIT_FAILURE);
    }
}

/**
 * Ls-tree command - tree di entries dikha!
 * (Ls-tree command - show tree's entries!)
 */
void handleLsTree(int argc, char* argv[]) {
    // Usage: ls-tree [--name-only] <tree-hash>
    
    if (argc < 3) {
        std::cerr << "Usage: ls-tree [--name-only] <tree-hash>\n";
        std::exit(EXIT_FAILURE);
    }
    
    bool name_only = false;
    std::string hash;
    
    // Flags parse karo
    // (Parse flags)
    if (argc == 4 && std::string(argv[2]) == "--name-only") {
        name_only = true;
        hash = argv[3];
    } else {
        hash = argv[2];
    }
    
    try {
        // Tree print karo
        // (Print tree)
        GitTree::printTree(hash, name_only);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::exit(EXIT_FAILURE);
    }
}

/**
 * Write-tree command - working directory toh tree bana!
 * (Write-tree command - create tree from working directory!)
 */
void handleWriteTree() {
    try {
        // Current directory toh tree banao
        // (Create tree from current directory)
        std::string hash = GitTree::writeTree(".");
        std::cout << hash << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::exit(EXIT_FAILURE);
    }
}

/**
 * Commit-tree command - commit object bana!
 * (Commit-tree command - create commit object!)
 */
void handleCommitTree(int argc, char* argv[]) {
    // Usage: commit-tree <tree-hash> [-p <parent-hash>] -m <message>
    // Parent is optional for first commit!
    // (Parent pehle commit ke liye optional hai!)
    
    if (argc < 4) {
        std::cerr << "Usage: commit-tree <tree-hash> [-p <parent-hash>] -m <message>\n";
        std::cerr << "Note: -p is optional for initial commit\n";
        std::exit(EXIT_FAILURE);
    }
    
    std::string tree_hash = argv[2];
    std::string parent_hash;
    std::string message;
    
    // Arguments parse karo - flags check karo
    // (Parse arguments - check flags)
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-p" && i + 1 < argc) {
            parent_hash = argv[++i];
        } else if (arg == "-m" && i + 1 < argc) {
            message = argv[++i];
        }
    }
    
    // Message zaroori hai - check karo!
    // (Message is required - check it!)
    if (message.empty()) {
        std::cerr << "Error: Commit message chahiye! (-m flag use karo)\n";
        std::cerr << "(Error: Need commit message! Use -m flag)\n";
        std::exit(EXIT_FAILURE);
    }
    
    try {
        // Environment toh author info lo
        // (Get author info from environment)
        auto [author_name, author_email] = GitCommit::getAuthorFromEnv();
        
        // Commit create karo
        // (Create commit)
        std::string commit_hash = GitCommit::createCommit(
            tree_hash,
            parent_hash,
            message,
            author_name,
            author_email
        );
        
        std::cout << commit_hash << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::exit(EXIT_FAILURE);
    }
}

/**
 * Clone command - remote repository local vich leke aa!
 * (Clone command - bring remote repository to local!)
 */
void handleClone(int argc, char* argv[]) {
    // Usage: clone <url> [directory]
    
    if (argc < 3) {
        std::cerr << "Usage: clone <repository-url> [directory]\n";
        std::exit(EXIT_FAILURE);
    }
    
    std::string url = argv[2];
    std::string target_dir = (argc >= 4) ? argv[3] : "";
    
    try {
        // Clone karo - saara magic Clone.hpp vich!
        // (Clone it - all magic in Clone.hpp!)
        if (!GitClone::clone(url, target_dir)) {
            std::exit(EXIT_FAILURE);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::exit(EXIT_FAILURE);
    }
}

// ==========================================
// MAIN FUNCTION - PROGRAM DA STARTING POINT
// (Main function - program's starting point)
// ==========================================

int main(int argc, char *argv[])
{
    // Output buffering band karo - instant results chahiye!
    // (Disable output buffering - want instant results!)
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // Shuru karde haan - Git da safar!
    // (Let's start - Git's journey!)
    std::cerr << "Punjabi Git - Coded with swag! 🚀\n";
    std::cerr << "(Git implementation with attitude!)\n\n";

    // Command check karo - koi dita bhi ya nahi?
    // (Check command - did they even give one?)
    if (argc < 2) {
        std::cerr << "Arre yaar, command toh do!"
                  << "\n(Come on buddy, give a command!)\n\n";
        std::cerr << "Available commands:\n";
        std::cerr << "  init                           - Initialize repository\n";
        std::cerr << "  cat-file -p <hash>            - Show object content\n";
        std::cerr << "  hash-object -w <file>         - Create blob from file\n";
        std::cerr << "  ls-tree [--name-only] <hash>  - List tree contents\n";
        std::cerr << "  write-tree                    - Write working directory as tree\n";
        std::cerr << "  commit-tree <tree> -p <parent> -m <msg> - Create commit\n";
        std::cerr << "  clone <url> [dir]             - Clone repository\n";
        return EXIT_FAILURE;
    }
    
    // Command extract karo - pehla argument
    // (Extract command - first argument)
    std::string command = argv[1];
    
    // Command routing - sahi handler nu bulao!
    // (Command routing - call the right handler!)
    try {
        if (command == "init") {
            // Repository initialize karo - neev rakho!
            // (Initialize repository - lay the foundation!)
            handleInit();
            
        } else if (command == "cat-file") {
            // Object content dikha - andar kya hai?
            // (Show object content - what's inside?)
            handleCatFile(argc, argv);
            
        } else if (command == "hash-object") {
            // File toh blob bana - store karo!
            // (Create blob from file - store it!)
            handleHashObject(argc, argv);
            
        } else if (command == "ls-tree") {
            // Tree entries dikha - files ki list!
            // (Show tree entries - list of files!)
            handleLsTree(argc, argv);
            
        } else if (command == "write-tree") {
            // Working directory toh tree - snapshot lo!
            // (Tree from working directory - take snapshot!)
            handleWriteTree();
            
        } else if (command == "commit-tree") {
            // Commit bana - history vich daal!
            // (Create commit - put in history!)
            handleCommitTree(argc, argv);
            
        } else if (command == "clone") {
            // Repository clone karo - copy leke aa!
            // (Clone repository - bring a copy!)
            handleClone(argc, argv);
            
        } else {
            // Unknown command - kya hai yeh?
            // (Unknown command - what is this?)
            std::cerr << "Unknown command: " << command 
                      << "\n(Eh command toh pata hi nahi!)\n";
            std::cerr << "Type without arguments to see available commands.\n";
            return EXIT_FAILURE;
        }
        
    } catch (const std::exception& e) {
        // Koi bhi error aayi - catch kar lo!
        // (Any error came - catch it!)
        std::cerr << "Fatal error: " << e.what() << "\n";
        std::cerr << "(Bada masla aa gaya!)\n";
        return EXIT_FAILURE;
    }
    
    // Sab theek - kaam mukka!
    // (All good - job done!)
    return EXIT_SUCCESS;
}
