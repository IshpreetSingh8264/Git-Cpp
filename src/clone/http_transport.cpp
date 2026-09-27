#include "clone/http_transport.hpp"

#include "utils/error.hpp"
#include "utils/process.hpp"
#include "utils/temp_file.hpp"

#include <cstdio>
#include <sstream>

namespace GitHttp {

namespace {

// Curl nu oh flags - quiet, error dikha de, connect te 30 second limit
// (The flags for curl - quiet, show errors, 30 second connect limit)
std::vector<std::string> baseCurlArgs() {
    return {"curl", "-s", "-S", "--connect-timeout", "30"};
}

std::string schemeFor(int port) {
    return port == 443 ? "https" : "http";
}

// "github.com" ya "127.0.0.1:8099" - scheme da default port chhad dein
// ("github.com" or "127.0.0.1:8099" - the scheme's default port is left off)
std::string authorityFor(const Endpoint& endpoint) {
    const int defaultPort = (endpoint.scheme == "https") ? 443 : 80;
    if (endpoint.port == defaultPort) {
        return endpoint.host;
    }
    return endpoint.host + ":" + std::to_string(endpoint.port);
}

/**
 * Curl chalao te usda output padh lo
 * (Run curl and read its output)
 */
std::string runCurl(const std::vector<std::string>& args) {
    GitUtil::TempFile out(".out");

    std::vector<std::string> argv = baseCurlArgs();
    for (const auto& arg : args) {
        argv.push_back(arg);
    }
    argv.push_back("-o");
    argv.push_back(out.path().string());

    int status = GitUtil::runProcess(argv);
    if (status != 0) {
        throw GitError::GitError("curl fail ho gaya (exit " + std::to_string(status) + ")");
    }

    return out.read();
}

} // namespace

Endpoint parseUrl(const std::string& url) {
    size_t protocolEnd = url.find("://");
    if (protocolEnd == std::string::npos) {
        throw GitError::GitError("URL galat hai, protocol nahi mila: " + url);
    }

    Endpoint endpoint;
    endpoint.scheme = url.substr(0, protocolEnd);
    if (endpoint.scheme == "https") {
        endpoint.port = 443;
    } else if (endpoint.scheme == "http") {
        endpoint.port = 80;
    } else {
        throw GitError::GitError("Ehi protocol support nahi hunda: " + endpoint.scheme);
    }

    size_t hostStart = protocolEnd + 3;
    size_t pathStart = url.find('/', hostStart);

    std::string authority = (pathStart == std::string::npos)
                                ? url.substr(hostStart)
                                : url.substr(hostStart, pathStart - hostStart);
    endpoint.path = (pathStart == std::string::npos) ? "/" : url.substr(pathStart);

    // Host te port alag: "github.com:8443"
    // (Host and port: "github.com:8443")
    size_t portSep = authority.rfind(':');
    if (portSep != std::string::npos) {
        endpoint.host = authority.substr(0, portSep);
        try {
            endpoint.port = std::stoi(authority.substr(portSep + 1));
        } catch (const std::exception&) {
            throw GitError::GitError("URL vich port galat hai: " + url);
        }
    } else {
        endpoint.host = authority;
    }

    if (endpoint.host.empty()) {
        throw GitError::GitError("URL vich host nahi mila: " + url);
    }
    return endpoint;
}

std::string buildUploadPackRequest(const std::vector<std::string>& wantHashes,
                                   const std::vector<std::string>& haveHashes) {
    std::ostringstream body;

    auto writeLine = [&body](const std::string& text) {
        char lengthText[8];
        std::snprintf(lengthText, sizeof(lengthText), "%04x",
                      static_cast<int>(text.size() + 4));
        body << lengthText << text;
    };

    for (const auto& hash : wantHashes) {
        writeLine("want " + hash + "\n");
    }

    // Havaal vich "have" lines bhej dein taaki server thin pack bhej sake
    // (Send have lines so the server is allowed to send a thin pack)
    for (const auto& hash : haveHashes) {
        writeLine("have " + hash + "\n");
    }

    body << "0000";              // flush
    body << "0009done\n";        // done

    return body.str();
}

std::string fetchRefAdvertisement(const Endpoint& endpoint) {
    std::string url = schemeFor(endpoint.port) + "://" + authorityFor(endpoint) + endpoint.path +
                      "/info/refs?service=git-upload-pack";

    return runCurl({url});
}

std::string fetchPack(const Endpoint& endpoint,
                      const std::vector<std::string>& wantHashes,
                      const std::vector<std::string>& haveHashes) {
    // Request body nu file vich likhna painda hai, kyunki binary data hai
    // (The request body has to go through a file, it is binary data)
    GitUtil::TempFile body(".req");
    body.write(buildUploadPackRequest(wantHashes, haveHashes));

    std::string url =
        schemeFor(endpoint.port) + "://" + authorityFor(endpoint) + endpoint.path + "/git-upload-pack";

    return runCurl({"-X",
                    "POST",
                    "-H",
                    "Content-Type: application/x-git-upload-pack-request",
                    "-H",
                    "Accept: application/x-git-upload-pack-result",
                    "--data-binary",
                    "@" + body.path().string(),
                    url});
}

} // namespace GitHttp
