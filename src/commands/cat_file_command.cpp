#include "commands/handlers.hpp"

#include "commit/commit_object.hpp"
#include "objects/object_store.hpp"
#include "tree/tree_object.hpp"
#include "utils/error.hpp"

#include <iostream>
#include <string>

namespace GitCommands {

void catFileCommand(const CommandContext& context) {
    // Usage: cat-file -p <hash>
    if (context.argumentCount() != 2 || context.argument(0) != "-p") {
        std::cerr << "Usage: cat-file -p <object-hash>\n";
        std::cerr << "Arre yaar, proper command toh use karo!"
                  << "\n(Come on buddy, use proper command!)\n";
        throw GitError::GitError("cat-file lai ke arguments galat ne");
    }

    std::string hash = context.argument(1);
    auto info = GitObject::getObjectInfo(".", hash);
    const std::string& type = info.first;

    if (type == "blob") {
        GitObject::printBlob(".", hash);
    } else if (type == "tree") {
        GitTree::printTree(".", hash, false);
    } else if (type == "commit") {
        GitCommit::printCommit(".", hash);
    } else {
        throw GitError::GitError("Unknown object type: " + type);
    }
}

} // namespace GitCommands
