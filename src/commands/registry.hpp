#pragma once

// ==========================================
// COMMAND REGISTRY
// (The command registry)
// ==========================================

#include "commands/handlers.hpp"

namespace GitCommands {

/**
 * Saare commands da map banaa
 * (Build the map of every command)
 *
 * Nawa command jodne lai sirf ek line: key te usda handler
 * (Adding a command is one line: its key and its handler)
 */
CommandRegistry buildRegistry();

} // namespace GitCommands
