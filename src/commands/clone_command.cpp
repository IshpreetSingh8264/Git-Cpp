#include "commands/handlers.hpp"

#include "clone/clone_operation.hpp"
#include "utils/error.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace GitCommands {

void cloneCommand(const CommandContext& context) {
    // Usage: clone <url> [directory]
    if (context.argumentCount() < 1 || context.argumentCount() > 2) {
        std::cerr << "Usage: clone <repository-url> [directory]\n";
        throw GitError::GitError("clone lai URL chahida hai");
    }

    std::string url = context.argument(0);
    std::string targetDir = context.argumentCount() == 2 ? context.argument(1) : "";

    if (!GitClone::clone(url, targetDir)) {
        // Clone khud sab da dawaad likh dinda hai, bass exit code bacha hai
        // (clone prints the diagnosis itself, we only owe it an exit code)
        std::exit(EXIT_FAILURE);
    }
}

} // namespace GitCommands
