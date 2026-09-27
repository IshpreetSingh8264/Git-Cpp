#include "commands/handlers.hpp"

#include "commit/commit_object.hpp"
#include "commit/identity.hpp"
#include "utils/error.hpp"

#include <iostream>
#include <string>

namespace GitCommands {

void commitTreeCommand(const CommandContext& context) {
    // Usage: commit-tree <tree-hash> [-p <parent-hash>] -m <message>
    // Pehle commit lai -p optional hai
    // (-p is optional for the first commit)
    std::string treeHash = context.argument(0);
    std::string parentHash;
    std::string message;

    for (size_t i = 2; i < context.args.size(); ++i) {
        const std::string& arg = context.args[i];
        if (arg == "-p" && i + 1 < context.args.size()) {
            parentHash = context.args[++i];
        } else if (arg == "-m" && i + 1 < context.args.size()) {
            message = context.args[++i];
        }
    }

    if (treeHash.empty() || message.empty()) {
        std::cerr << "Usage: commit-tree <tree-hash> [-p <parent-hash>] -m <message>\n";
        std::cerr << "Note: -p is optional for initial commit\n";
        throw GitError::GitError("commit-tree lai tree hash te -m message dono chahiye");
    }

    std::cout << GitCommit::createCommit(".", treeHash, parentHash, message,
                                          GitCommit::identityFromEnv())
              << "\n";
}

} // namespace GitCommands
