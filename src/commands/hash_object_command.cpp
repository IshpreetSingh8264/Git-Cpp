#include "commands/handlers.hpp"

#include "objects/object_store.hpp"
#include "utils/error.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace GitCommands {

void hashObjectCommand(const CommandContext& context) {
    // Usage: hash-object -w <file>
    if (context.argumentCount() != 2 || context.argument(0) != "-w") {
        std::cerr << "Usage: hash-object -w <file>\n";
        throw GitError::GitError("hash-object lai ke arguments galat ne");
    }

    std::string filename = context.argument(1);

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw GitError::GitError("File nahi khul sakdi: " + filename);
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::cout << GitObject::createBlob(".", content) << "\n";
}

} // namespace GitCommands
