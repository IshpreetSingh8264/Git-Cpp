#include "commands/handlers.hpp"

#include "tree/tree_object.hpp"
#include "utils/error.hpp"

#include <iostream>

namespace GitCommands {

void writeTreeCommand(const CommandContext& context) {
    if (context.argumentCount() > 0) {
        std::cerr << "Usage: write-tree\n";
        throw GitError::GitError("write-tree koi argument nahi lenda");
    }

    std::cout << GitTree::writeTree(".", ".") << "\n";
}

} // namespace GitCommands
