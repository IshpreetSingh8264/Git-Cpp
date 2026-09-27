#pragma once

// ==========================================
// COMMAND DA CONTRACT
// (The command contract)
// ==========================================

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace GitCommands {

/**
 * Ek command nu ki mila
 * (What one command was handed)
 */
struct CommandContext {
    // args[0] hamesha command da naam hai, uske baad usde arguments
    // (args[0] is always the command name, the rest are its arguments)
    std::vector<std::string> args;

    /**
     * Usda argument chakh lo, nahi te khaali string
     * (Pick one argument out, or an empty string)
     *
     * index 0 pehla argument hai, command te program ginti nahi
     * (index 0 is the first argument; the command and program do not count)
     */
    const std::string& argument(size_t index) const;

    /**
     * Command ke baad kitne arguments ne
     * (How many arguments came after the command)
     */
    size_t argumentCount() const { return args.size() > 2 ? args.size() - 2 : 0; }
};

// Rule 6: har command da ek hi roop
// (Rule 6: one shape for every command)
using Handler = std::function<void(const CommandContext&)>;

// Rule 5: command add karna mane ik key add karna
// (Rule 5: adding a command means adding one key)
using CommandRegistry = std::map<std::string, Handler>;

// har command da apna kaam - ek ek file te
// (each command's own job - one per file)
void initCommand(const CommandContext& context);
void catFileCommand(const CommandContext& context);
void hashObjectCommand(const CommandContext& context);
void lsTreeCommand(const CommandContext& context);
void writeTreeCommand(const CommandContext& context);
void commitTreeCommand(const CommandContext& context);
void cloneCommand(const CommandContext& context);

} // namespace GitCommands
