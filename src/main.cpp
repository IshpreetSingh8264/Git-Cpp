// ==========================================
// PUNJABI GIT - ASLI SWAG NAAL!
// (Punjabi Git - with real swag!)
// ==========================================
// Git da dil sirf eh hai: arguments ik jagah dasso te dispatcher nu
// de do. Baqi sab kaam commands te modules de bujhich hai.
// (This file's whole job: hand the arguments to the dispatcher. Every
//  other bit of work lives in the commands and the modules.)
//
// Modules: repository, objects, tree, commit, clone
// ==========================================

#include "commands/dispatcher.hpp"

#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(argc));

    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    return GitCommands::dispatch(args);
}
