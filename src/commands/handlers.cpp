#include "commands/handlers.hpp"

namespace GitCommands {

namespace {
const std::string kEmpty;
}

const std::string& CommandContext::argument(size_t index) const {
    // args[0] program, args[1] command - dono nahi ginne
    // (args[0] is the program, args[1] is the command, neither is counted)
    if (index + 2 >= args.size()) {
        return kEmpty;
    }
    return args[index + 2];
}

} // namespace GitCommands
