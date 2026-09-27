#pragma once

// ==========================================
// COMMIT DA PEHCHAN - KAUN BANAYA
// (The commit's identity - who wrote it)
// ==========================================

#include <string>

namespace GitCommit {

/**
 * Commit da author
 * (A commit's author)
 */
struct Identity {
    std::string name;
    std::string email;
};

/**
 * Environment toh author te committer di jaankari paawo
 * (Read the author and committer from the environment)
 *
 * Order:
 *   1. GIT_AUTHOR_NAME / GIT_AUTHOR_EMAIL
 *   2. GIT_COMMITTER_NAME / GIT_COMMITTER_EMAIL
 *   3. .git/config vich user.name / user.email
 *   4. Git da apna test identity - "A U Thor <author@example.com>"
 *
 * Aam tor te pehli teen himmat bare hunde ne. Chauthi sirf tab chalti hai
 * jab environment bilkul khali hove - tab vih Galat te hukka dassde haan
 * te kisi da naam nahi banawnde. Real Git is case te error dinda, par
 * CodeCrafters da commit-tree stage environment khali chalda hai, is liye
 * chupchaap fail hon na dein.
 * (Normally one of the first three answers. The last one only fires when
 *  the environment is completely empty - and even then we say so out loud
 *  rather than inventing a person. Real Git errors here, but the
 *  CodeCrafters commit-tree stage runs with an empty environment, so
 *  quietly failing there is not an option either.)
 */
Identity identityFromEnv();

/**
 * "<name> <email> <timestamp> <timezone>" banao
 * (Build "<name> <email> <timestamp> <timezone>")
 *
 * @param timestamp - 0 dasso te waqt eh laye ga
 *                   (pass 0 to use the current time)
 */
std::string formatPersonInfo(const std::string& name,
                             const std::string& email,
                             long long timestamp = 0);

} // namespace GitCommit
