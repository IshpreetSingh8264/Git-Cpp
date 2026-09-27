#pragma once

// ==========================================
// BASH NAHI, DIRECT CHILD PROCESS
// (Not a shell - a direct child process)
// ==========================================

#include <string>
#include <vector>

namespace GitUtil {

/**
 * Program nu chala ke usda exit code return karda hai
 * (Runs a program and returns its exit code)
 *
 * system() te /bin/sh use karda hai, is nu:
 *  - argument vich shell metacharacters (;, |, $, ") nu control karo
 *  - shell injection te SIGINT de dono problems
 * iss nu fork + execvp use karda hai, koi shell nahi.
 * (system() goes through /bin/sh, which means shell metacharacters
 *  in an argument are dangerous and a Ctrl-C kills us too.
 *  This uses fork + execvp, so there is no shell at all.)
 *
 * @param argv - program naam te usde arguments, argv[0] naal
 *              (program name and its arguments, including argv[0])
 * @return exit code, ya -1 agar program chala hi nahi sakey
 *         (exit code, or -1 if the program could not be started)
 */
int runProcess(const std::vector<std::string>& argv);

} // namespace GitUtil
