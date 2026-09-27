#pragma once

// ==========================================
// REF ADVERTISEMENT - SERVER DA HAAL
// (Ref advertisement - what the server has)
// ==========================================

#include <string>
#include <vector>

namespace GitClone {

/**
 * Server da ek ref
 * (One ref from the server)
 */
struct RefLine {
    std::string hash;                 // 40 character object id
    std::string name;                 // "refs/heads/main" ya "HEAD"
    std::vector<std::string> capabilities;  // sirf pehle line te hundiyan
};

/**
 * Poori advertisement
 * (The whole advertisement)
 */
struct RefAdvertisement {
    std::vector<RefLine> refs;
    std::string headTarget;   // "symref=HEAD:refs/heads/main" toh mila
    std::string defaultBranch;  // "refs/heads/main", ya khaali

    /**
     * Je sab objects humein chahiye, order vich, duplicate naal
     * (Every object we need, in order, without duplicates)
     */
    std::vector<std::string> wantedHashes() const;

    /**
     * Default branch da naam, "refs/heads/" hatake - "main"
     * (The default branch name with refs/heads/ stripped - "main")
     */
    std::string defaultBranchName() const;

    /**
     * Default branch da commit hash
     * (The default branch's commit hash)
     */
    std::string defaultBranchHash() const;
};

/**
 * info/refs da response padh ke advertisement banaa
 * (Read the info/refs response and build the advertisement)
 *
 * Default branch pehlan "symref=HEAD:..." toh dhoondha janda hai, phir
 * main, phir master, phir pehli branch. Is nu "master"/"main" te hi
 * atak nahi dende - server jo kehta hai oh sun lo.
 * (The default branch is found from "symref=HEAD:..." first, then main,
 *  then master, then the first branch. Do not get stuck on master/main -
 *  listen to what the server says.)
 */
RefAdvertisement parseRefAdvertisement(const std::string& response);

} // namespace GitClone
