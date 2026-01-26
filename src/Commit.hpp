#pragma once

#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "GitObject.hpp"

// ==========================================
// COMMIT OBJECTS DA JADU
// (The magic of commit objects)
// ==========================================

namespace GitCommit {

/**
 * Commit message banane da helper function
 * (Helper function to create commit message)
 * 
 * Timestamp te timezone ke naal formatted karda hai
 * (Formats with timestamp and timezone)
 * 
 * @param name - Author/Committer naam (name)
 * @param email - Email address
 * @param timestamp - Unix timestamp (optional, current time if not provided)
 * @return Formatted "name <email> timestamp timezone" string
 */
inline std::string formatPersonInfo(const std::string& name, 
                                    const std::string& email,
                                    time_t timestamp = 0) {
    // Agar timestamp nahi dita, toh current time lo
    // (If timestamp not given, take current time)
    if (timestamp == 0) {
        timestamp = std::time(nullptr);
    }
    
    // Timezone calculate karo - local time vs UTC
    // (Calculate timezone - local time vs UTC)
    std::tm* tm_local = std::localtime(&timestamp);
    std::tm tm_utc = *std::gmtime(&timestamp);
    
    // Timezone offset in seconds
    time_t local_time = std::mktime(tm_local);
    time_t utc_time = std::mktime(&tm_utc);
    int offset_seconds = static_cast<int>(std::difftime(local_time, utc_time));
    
    // Offset nu hours te minutes vich convert karo
    // (Convert offset to hours and minutes)
    int offset_hours = offset_seconds / 3600;
    int offset_minutes = (std::abs(offset_seconds) % 3600) / 60;
    
    // Timezone string banao - "+0530" ya "-0800" vaang
    // (Create timezone string - like "+0530" or "-0800")
    std::stringstream tz;
    tz << (offset_seconds >= 0 ? "+" : "-")
       << std::setw(2) << std::setfill('0') << std::abs(offset_hours)
       << std::setw(2) << std::setfill('0') << offset_minutes;
    
    // Full formatted string return karo
    // (Return full formatted string)
    std::stringstream ss;
    ss << name << " <" << email << "> " << timestamp << " " << tz.str();
    
    return ss.str();
}

/**
 * Commit object create karne da main function
 * (Main function to create commit object)
 * 
 * Commit = snapshot of your work at a point in time
 * Bilkul photo kheechni vaang - ek moment capture karo!
 * (Commit = snapshot of your work at a point in time
 *  Just like clicking a photo - capture a moment!)
 * 
 * @param tree_hash - Tree object da hash
 * @param parent_hash - Parent commit hash (empty for first commit)
 * @param message - Commit message
 * @param author_name - Author naam (name)
 * @param author_email - Author email
 * @param committer_name - Committer naam (optional, uses author if empty)
 * @param committer_email - Committer email (optional, uses author if empty)
 * @return SHA-1 hash of created commit
 */
inline std::string createCommit(const std::string& tree_hash,
                                const std::string& parent_hash,
                                const std::string& message,
                                const std::string& author_name,
                                const std::string& author_email,
                                const std::string& committer_name = "",
                                const std::string& committer_email = "") {
    // Commit content banao - Git format follow karo!
    // (Create commit content - follow Git format!)
    std::stringstream commit_content;
    
    // Tree reference - required!
    // (Tree reference - required!)
    commit_content << "tree " << tree_hash << "\n";
    
    // Parent reference - agar first commit nahi hai
    // (Parent reference - if not first commit)
    if (!parent_hash.empty()) {
        commit_content << "parent " << parent_hash << "\n";
    }
    
    // Author info - kon banaya?
    // (Author info - who created?)
    time_t now = std::time(nullptr);
    commit_content << "author " 
                   << formatPersonInfo(author_name, author_email, now) 
                   << "\n";
    
    // Committer info - agar alag hai toh
    // (Committer info - if different)
    std::string commit_name = committer_name.empty() ? author_name : committer_name;
    std::string commit_email = committer_email.empty() ? author_email : committer_email;
    commit_content << "committer " 
                   << formatPersonInfo(commit_name, commit_email, now) 
                   << "\n";
    
    // Empty line te phir message - Git da format!
    // (Empty line then message - Git's format!)
    commit_content << "\n" << message << "\n";
    
    // Commit object write karo
    // (Write commit object)
    return GitObject::writeObject("commit", commit_content.str());
}

/**
 * Commit object parse karne da function
 * (Function to parse commit object)
 * 
 * @param hash - Commit object hash
 * @return Commit content as string
 */
inline std::string readCommit(const std::string& hash) {
    // Object padho
    // (Read object)
    std::string content = GitObject::readObject(hash);
    
    // Header check karo
    // (Check header)
    size_t null_pos = content.find('\0');
    if (null_pos == std::string::npos) {
        throw std::runtime_error("Invalid commit format!");
    }
    
    std::string header = content.substr(0, null_pos);
    if (header.substr(0, 6) != "commit") {
        throw std::runtime_error("Eh commit nahi hai yaar!"
                               "\n(This ain't a commit buddy!)");
    }
    
    return content.substr(null_pos + 1);
}

/**
 * Commit object print karne da function - detailed info
 * (Function to print commit object - detailed info)
 * 
 * @param hash - Commit hash
 */
inline void printCommit(const std::string& hash) {
    std::string commit_content = readCommit(hash);
    
    // Saari jaankari print karo - full details!
    // (Print all information - full details!)
    std::cout << "commit " << hash << "\n";
    std::cout << commit_content;
}

/**
 * Commit toh tree hash extract karne da function
 * (Function to extract tree hash from commit)
 * 
 * @param hash - Commit hash
 * @return Tree hash
 */
inline std::string getCommitTree(const std::string& hash) {
    std::string commit_content = readCommit(hash);
    
    // "tree " line dhundho
    // (Find "tree " line)
    size_t tree_pos = commit_content.find("tree ");
    if (tree_pos == std::string::npos) {
        throw std::runtime_error("Commit vich tree nahi mila!"
                               "\n(Tree not found in commit!)");
    }
    
    // Hash extract karo - newline tak
    // (Extract hash - until newline)
    size_t newline_pos = commit_content.find('\n', tree_pos);
    std::string tree_line = commit_content.substr(tree_pos, newline_pos - tree_pos);
    
    // "tree " ke baad jo hai, woh hash hai
    // (Whatever is after "tree " is the hash)
    return tree_line.substr(5); // "tree " = 5 characters
}

/**
 * Environment variables toh author info lene da function
 * (Function to get author info from environment variables)
 * 
 * @return Pair of (name, email)
 */
inline std::pair<std::string, std::string> getAuthorFromEnv() {
    // Environment variables check karo
    // (Check environment variables)
    const char* name_env = std::getenv("GIT_AUTHOR_NAME");
    const char* email_env = std::getenv("GIT_AUTHOR_EMAIL");
    
    std::string name = name_env ? name_env : "Punjabi Coder";
    std::string email = email_env ? email_env : "coder@punjab.dev";
    
    return {name, email};
}

} // namespace GitCommit
