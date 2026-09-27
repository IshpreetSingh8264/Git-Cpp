#pragma once

// ==========================================
// WORKING TREE BANANA - CHECKOUT
// (Materialising the working tree - checkout)
// ==========================================

#include <filesystem>
#include <string>
#include <vector>

namespace GitWorktree {

/**
 * Checkout da hisaab
 * (The checkout tally)
 */
struct Report {
    std::string branch;   // je branch checkout hui
    std::string commit;   // je commit checkout hui
    std::string tree;     // je tree(hash) khuli
    size_t directories = 0;
    size_t files = 0;
    size_t symlinks = 0;
    std::vector<std::string> missing;  // je object store vich hi nahi mile

    /**
     * Koi object chhut gaya?
     * (Did any object go missing?)
     */
    bool complete() const { return missing.empty(); }
};

/**
 * HEAD nu padh ke usda commit checkout karo
 * (Read HEAD and check out the commit it points at)
 *
 * HEAD symbolic te usdi ref file padhi jandi hai
 * (HEAD may be symbolic, in which case its ref file is read)
 *
 * @throws GitError agar koi blob ya tree object na mile - khaali file
 *         bana ke chup nahi karenge, kyunki oh corrupt working tree hai
 *         (throws GitError if any blob or tree object is missing. We
 *          never paper over it with an empty file, because that is a
 *          corrupt working tree.)
 */
Report checkoutHead(const std::filesystem::path& root);

/**
 * Ek commit da poora working tree banaa
 * (Materialise the whole working tree of one commit)
 */
Report checkoutCommit(const std::filesystem::path& root, const std::string& commitHash);

} // namespace GitWorktree
