#include "commit/identity.hpp"

#include "repository/repository.hpp"
#include "utils/error.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace GitCommit {

namespace {

// Environment variable nu khaali te na mile te empty return karo
// (Read an env var, treating unset and empty as the same thing)
std::string envOrEmpty(const char* key) {
    const char* value = std::getenv(key);
    if (value == nullptr || value[0] == '\0') {
        return "";
    }
    return value;
}

std::string trim(const std::string& text) {
    size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::string stripSpaces(const std::string& text) {
    std::string out = text;
    out.erase(std::remove_if(out.begin(), out.end(),
                             [](unsigned char c) { return std::isspace(c) != 0; }),
              out.end());
    return out;
}

// .git/config vich user.name ya user.email
// (user.name or user.email out of .git/config)
std::string configUser(const char* key) {
    try {
        std::ifstream file(GitRepository::getGitDir() / "config");
        if (!file) {
            return "";
        }
        std::string wanted = std::string("user.") + key;
        std::string section;
        std::string line;
        while (std::getline(file, line)) {
            std::string trimmed = trim(line);
            if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
                continue;
            }
            if (trimmed.front() == '[' && trimmed.back() == ']') {
                section = trim(trimmed.substr(1, trimmed.size() - 2));
                continue;
            }

            size_t equals = line.find('=');
            if (equals == std::string::npos) {
                continue;
            }
            // Bade forms: "user.name" ya [user] ke andar "name"
            // (Both shapes: "user.name", or "name" inside a [user] section)
            std::string left = stripSpaces(line.substr(0, equals));
            if (left != wanted && !(section == "user" && left == key)) {
                continue;
            }

            std::string value = trim(line.substr(equals + 1));
            if (!value.empty()) {
                return value;
            }
        }
    } catch (const std::exception&) {
        // Repository nahi hai ya config nahi padhi ja sakdi - koi galat nahi
        // (No repository, or the config will not read - not an error)
    }
    return "";
}

// Local time toh UTC da offset, "+0530" vaang
// (Local time's offset from UTC, like "+0530")
std::string localZone(long long timestamp) {
    time_t raw = static_cast<time_t>(timestamp);
    std::tm local{};
    std::tm utc{};
    ::localtime_r(&raw, &local);
    ::gmtime_r(&raw, &utc);

    long long offsetSeconds = static_cast<long long>(std::mktime(&local)) -
                              static_cast<long long>(std::mktime(&utc));

    long long absolute = offsetSeconds < 0 ? -offsetSeconds : offsetSeconds;
    long long hours = absolute / 3600;
    long long minutes = (absolute % 3600) / 60;

    std::ostringstream out;
    out << (offsetSeconds >= 0 ? "+" : "-")
        << std::setw(2) << std::setfill('0') << hours
        << std::setw(2) << std::setfill('0') << minutes;
    return out.str();
}

} // namespace

Identity identityFromEnv() {
    std::string name = envOrEmpty("GIT_AUTHOR_NAME");
    std::string email = envOrEmpty("GIT_AUTHOR_EMAIL");

    if (name.empty() || email.empty()) {
        std::string commitName = envOrEmpty("GIT_COMMITTER_NAME");
        std::string commitEmail = envOrEmpty("GIT_COMMITTER_EMAIL");
        if (!commitName.empty() && !commitEmail.empty()) {
            return {commitName, commitEmail};
        }
    }

    if (name.empty()) {
        name = configUser("name");
    }
    if (email.empty()) {
        email = configUser("email");
    }

    if (name.empty() || email.empty()) {
        // Bilkul kuch nahi mila. Kisi da naam nahi banaunda - Git da apna
        // test identity bana ke galat dassde haan
        // (Nothing at all. We do not invent a person, we use Git's own test
        //  identity and say that is what we did)
        std::cerr << "Warning: GIT_AUTHOR_NAME/GIT_AUTHOR_EMAIL set nahi ne, te .git/config "
                     "vich user.name/user.email vich kuch nahi mila. "
                     "'A U Thor <author@example.com>' use kar rahe haan.\n"
                     "(Warning: no GIT_AUTHOR_* in the environment and nothing in .git/config. "
                     "Falling back to 'A U Thor <author@example.com>'.)\n";
        return {"A U Thor", "author@example.com"};
    }

    return {name, email};
}

std::string formatPersonInfo(const std::string& name,
                             const std::string& email,
                             long long timestamp) {
    if (timestamp == 0) {
        timestamp = static_cast<long long>(std::time(nullptr));
    }

    char stamp[32];
    std::snprintf(stamp, sizeof(stamp), "%lld", timestamp);
    return name + " <" + email + "> " + stamp + " " + localZone(timestamp);
}

} // namespace GitCommit
