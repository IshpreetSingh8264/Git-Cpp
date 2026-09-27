#include "commands/dispatcher.hpp"

#include "commands/registry.hpp"
#include "utils/error.hpp"

#include <cstdlib>
#include <iostream>

namespace GitCommands {

void printUsage() {
    std::cerr << "Arre yaar, command toh do!"
              << "\n(Come on buddy, give a command!)\n\n";
    std::cerr << "Available commands:\n";
    std::cerr << "  init                           - Initialize repository\n";
    std::cerr << "  cat-file -p <hash>            - Show object content\n";
    std::cerr << "  hash-object -w <file>         - Create blob from file\n";
    std::cerr << "  ls-tree [--name-only] <hash>  - List tree contents\n";
    std::cerr << "  write-tree                    - Write working directory as tree\n";
    std::cerr << "  commit-tree <tree> -p <parent> -m <msg> - Create commit\n";
    std::cerr << "  clone <url> [dir]             - Clone repository\n";
}

int dispatch(const std::vector<std::string>& argv) {
    // Turant dikha dein - CodeCrafters te output turant chahida hunda
    // (Show it at once - CodeCrafters needs unbuffered output)
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cerr << "Punjabi Git - Coded with swag! 🚀\n";
    std::cerr << "(Git implementation with attitude!)\n\n";

    if (argv.size() < 2) {
        printUsage();
        return EXIT_FAILURE;
    }

    const CommandRegistry registry = buildRegistry();
    auto found = registry.find(argv[1]);
    if (found == registry.end()) {
        std::cerr << "Unknown command: " << argv[1] << "\n(Eh command toh pata hi nahi!)\n";
        std::cerr << "Type without arguments to see available commands.\n";
        return EXIT_FAILURE;
    }

    CommandContext context;
    context.args = argv;

    try {
        found->second(context);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

} // namespace GitCommands
