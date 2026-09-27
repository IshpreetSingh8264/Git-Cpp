#include "utils/process.hpp"

#include <cerrno>
#include <cstring>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

namespace GitUtil {

int runProcess(const std::vector<std::string>& argv) {
    if (argv.empty()) {
        return -1;
    }

    // execvp nu char* const[] chahida, te lifetime is call tak hi hai
    // (execvp wants a char* const[], and its lifetime is just this call)
    std::vector<char*> raw;
    raw.reserve(argv.size() + 1);
    for (const auto& arg : argv) {
        raw.push_back(const_cast<char*>(arg.c_str()));
    }
    raw.push_back(nullptr);

    // -1 te SIGPEDE (signal #) wapas milda hai, waitpid te exit status
    // (A -1 return carries the signal number, waitpid gives the exit status)
    pid_t pid = ::fork();
    if (pid < 0) {
        return -1;
    }

    if (pid == 0) {
        // Bacche vich: seedha program chalao, koi shell nahi
        // (In the child: exec straight into the program, no shell)
        ::execvp(raw[0], raw.data());
        // Sirf tabhi pohncha jada jado exec fail ho gaya
        // (Only reached when exec failed)
        ::_exit(127);
    }

    int status = 0;
    while (::waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            return -1;
        }
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

} // namespace GitUtil
