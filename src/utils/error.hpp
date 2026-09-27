#pragma once

// ==========================================
// IK GALAT DI DA REPORTER
// (The reporter of our mistakes)
// ==========================================
// Pure contract header: no definitions, so no .cpp is needed
// (Like every types/ header in the Shell reference - zero logic.)

#include <stdexcept>
#include <string>

namespace GitError {

/**
 * Saare recoverable galat eh class vich aunde ne
 * (All recoverable errors arrive in this class)
 *
 * Galat ya te pakka rehne wale step te throw karo - main
 * oh nu pakad ke exit code set karda hai
 * (Throw on any step that definitely failed - main catches it
 *  and sets the exit code)
 */
class GitError : public std::runtime_error {
public:
    explicit GitError(const std::string& what) : std::runtime_error(what) {}
};

} // namespace GitError
