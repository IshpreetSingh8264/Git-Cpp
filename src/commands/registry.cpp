#include "commands/registry.hpp"

namespace GitCommands {

CommandRegistry buildRegistry() {
    // Koi switch nahi, koi if-chain nahi - sirf data
    // (No switch, no if-chain - just data)
    return {
        {"init", initCommand},
        {"cat-file", catFileCommand},
        {"hash-object", hashObjectCommand},
        {"ls-tree", lsTreeCommand},
        {"write-tree", writeTreeCommand},
        {"commit-tree", commitTreeCommand},
        {"clone", cloneCommand},
    };
}

} // namespace GitCommands
