#include "commands/handlers.hpp"

#include "tree/tree_object.hpp"
#include "utils/error.hpp"

#include <iostream>
#include <string>

namespace GitCommands {

void lsTreeCommand(const CommandContext& context) {
    // Usage: ls-tree [--name-only] <tree-hash>
    //
    // Flags kade vich aa sakte ne, is liye har argument nu dekho. Pehlan
    // woh sirf tab hi samajhda si jab argc theek hi hoy.
    // (Flags may come in any position, so we inspect every argument. It
    //  used to understand one only when argc happened to be exactly right.)
    bool nameOnly = false;
    std::string hash;
    std::string unknownFlag;

    // args[0] te args[1] program te command ne - yahan toh 2 toh shuru
    // (args[0] and args[1] are the program and the command, so start at 2)
    for (size_t i = 2; i < context.args.size(); ++i) {
        const std::string& arg = context.args[i];
        if (arg == "--name-only") {
            nameOnly = true;
        } else if (arg.rfind("--", 0) == 0) {
            if (unknownFlag.empty()) {
                unknownFlag = arg;
            }
        } else if (hash.empty()) {
            hash = arg;
        } else {
            std::cerr << "Usage: ls-tree [--name-only] <tree-hash>\n";
            throw GitError::GitError("ls-tree lai ek hi hash dena chahida hai");
        }
    }

    if (!unknownFlag.empty()) {
        std::cerr << "Usage: ls-tree [--name-only] <tree-hash>\n";
        throw GitError::GitError("ls-tree da flag nahi pehchana: " + unknownFlag);
    }

    if (hash.empty()) {
        std::cerr << "Usage: ls-tree [--name-only] <tree-hash>\n";
        throw GitError::GitError("ls-tree lai tree hash chahida hai");
    }

    GitTree::printTree(".", hash, nameOnly);
}

} // namespace GitCommands
