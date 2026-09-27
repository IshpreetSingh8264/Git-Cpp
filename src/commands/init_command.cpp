#include "commands/handlers.hpp"

#include "repository/repository.hpp"
#include "utils/error.hpp"

#include <iostream>

namespace GitCommands {

void initCommand(const CommandContext& context) {
    if (context.argumentCount() > 0) {
        std::cerr << "Usage: init\n";
        return;
    }

    if (!GitRepository::init(".")) {
        throw GitError::GitError("Repository initialize nahi ho sakda");
    }
}

} // namespace GitCommands
