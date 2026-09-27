#pragma once

// ==========================================
// HTTP TRANSPORT - CURL TE
// (HTTP transport - via curl)
// ==========================================

#include <string>
#include <vector>

namespace GitHttp {

/**
 * Server da address
 * (The server's address)
 */
struct Endpoint {
    std::string scheme;   // http ya https
    std::string host;     // github.com
    int port = 443;       // 443 ya 80
    std::string path;     // /codecrafters-io/git-sample-3
};

/**
 * Repository URL nu tod de - protocol, host, port, path
 * (Take a repository URL apart - protocol, host, port, path)
 *
 * @throws GitError agar URL valid nahi hai ya protocol support nahi hunda
 */
Endpoint parseUrl(const std::string& url);

/**
 * info/refs GET karo - server kya kya haunda hai
 * (GET info/refs - what the server has)
 */
std::string fetchRefAdvertisement(const Endpoint& endpoint);

/**
 * git-upload-pack nu POST karo te pack bytes waapas lo
 * (POST to git-upload-pack and get the pack bytes back)
 *
 * @param wantHashes - je objects chahiye
 * @param haveHashes - je objects humein already milda (havaal thin pack
 *                     banaunda hai server lai, is liye eh khavar rakhde ne)
 *                    (the objects we already have, which is what lets
 *                     the server build a thin pack)
 */
std::string fetchPack(const Endpoint& endpoint,
                      const std::vector<std::string>& wantHashes,
                      const std::vector<std::string>& haveHashes);

/**
 * Upload-pack request da body banao - pkt-line format
 * (Build an upload-pack request body - pkt-line format)
 */
std::string buildUploadPackRequest(const std::vector<std::string>& wantHashes,
                                   const std::vector<std::string>& haveHashes);

} // namespace GitHttp
