#pragma once

// ==========================================
// DISPATCHER - PROGRAM DA DARWAZA
// (The dispatcher - the program's front door)
// ==========================================

#include <string>
#include <vector>

namespace GitCommands {

/**
 * Usvaal nu command tak pohncha de
 * (Get the request to a command)
 *
 * Yehi layer sab cross-cutting kaam sambhalndi hai - banner, usage
 * message, error pakadna, exit code. Commands sirf apna kaam karde ne.
 * (This layer owns all the cross-cutting work - the banner, the usage
 *  message, catching errors, the exit code. Commands just do their job.)
 *
 * @param argv - poora command line, argv[0] program da naam
 *               (the whole command line, argv[0] being the program name)
 * @return process da exit code
 *         (the process exit code)
 */
int dispatch(const std::vector<std::string>& argv);

/**
 * Galat ke baare te sahi likha lya likhne da message
 * (What to print when something went wrong)
 */
void printUsage();

} // namespace GitCommands
