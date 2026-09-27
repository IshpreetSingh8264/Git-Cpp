#include "clone/clone_operation.hpp"

#include "clone/http_transport.hpp"
#include "clone/pack_reader.hpp"
#include "clone/ref_advertisement.hpp"
#include "clone/worktree_writer.hpp"
#include "repository/repository.hpp"
#include "utils/error.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <vector>

namespace GitClone {

namespace {

// REF_DELTA de base hash pakad de, te unnu alag maang dein
// (Round after round we ask for the REF_DELTA bases we could not find)
constexpr int kMaxFetchRounds = 5;

// Server nu kitne "have" lines bhejne ne
// (How many have lines to send the server)
constexpr size_t kMaxHaves = 32;

std::string directoryNameFromUrl(const std::string& url) {
    size_t lastSlash = url.find_last_of('/');
    std::string name = lastSlash == std::string::npos ? url : url.substr(lastSlash + 1);

    if (name.size() > 4 && name.compare(name.size() - 4, 4, ".git") == 0) {
        name = name.substr(0, name.size() - 4);
    }
    if (name.empty()) {
        name = "repo";
    }
    return name;
}

void logPackStats(const std::string& label, const GitPack::Stats& stats) {
    std::cerr << label << ": " << stats.objectsWritten << " object bane pack vich toh"
              << " (" << stats.baseObjects << " raw + " << stats.deltaObjects << " delta, "
              << stats.deltasResolved << " delta resolve hoye)"
              << "\n  (" << stats.objectsWritten << " objects written from the pack: "
              << stats.baseObjects << " raw + " << stats.deltaObjects << " delta, "
              << stats.deltasResolved << " deltas resolved)\n";

    for (const auto& failure : stats.failures) {
        std::cerr << "  pack object fail: " << failure << "\n";
    }
}

std::vector<std::string> uniqueMissing(const std::vector<std::string>& hashes) {
    std::vector<std::string> out;
    for (const auto& hash : hashes) {
        if (std::find(out.begin(), out.end(), hash) == out.end()) {
            out.push_back(hash);
        }
    }
    return out;
}

// Har advertised branch te tag da ref likh de
// (Write a ref for every advertised branch and tag)
void writeAllRefs(const std::filesystem::path& root, const RefAdvertisement& advertisement) {
    size_t written = 0;
    size_t skipped = 0;
    for (const auto& ref : advertisement.refs) {
        // "HEAD" sirf ik pseudo-ref hai, usda alag file nahi hunda
        // ("HEAD" is only a pseudo-ref, it gets no file of its own)
        if (ref.name == "HEAD" || ref.name.rfind("refs/", 0) != 0) {
            ++skipped;
            continue;
        }

        // "refs/tags/v1.2^{}" eh peeled tag da signal hai, ref nahi. Iko
        // likhne te git usda naam hi reject kar dinda hai
        // ("refs/tags/v1.2^{}" marks a peeled tag rather than being a ref.
        //  Writing it produces a name real git rejects)
        if (ref.name.size() > 3 && ref.name.compare(ref.name.size() - 3, 3, "^{}") == 0) {
            ++skipped;
            continue;
        }

        std::filesystem::path refPath = GitRepository::getRefsDir(root) /
                                         ref.name.substr(std::string("refs/").size());
        std::filesystem::create_directories(refPath.parent_path());

        std::ofstream refFile(refPath, std::ios::binary | std::ios::trunc);
        if (!refFile) {
            throw GitError::GitError("Ref likh nahi sakdi: " + refPath.string());
        }
        refFile << ref.hash << "\n";
        ++written;
    }
    std::cerr << "Refs written: " << written << (skipped > 0 ? " (" + std::to_string(skipped) +
                                                               " pseudo-ref skipped)"
                                                         : "")
              << "\n";
}

// Default branch da ref likh de, te HEAD oh di taraf point karwa de
// (Write the default branch's ref and point HEAD at it)
void writeHead(const std::filesystem::path& root, const RefAdvertisement& advertisement) {
    if (advertisement.defaultBranch.empty() || advertisement.defaultBranchHash().empty()) {
        throw GitError::GitError(
            "Server te koi branch nahi mili - khali repository hai ya asaan nahi padhi ja sakdi. "
            "(No branch found on the server - the repository is empty, or we could not read it.)");
    }

    std::string branch = advertisement.defaultBranchName();
    std::string hash = advertisement.defaultBranchHash();

    std::ofstream headFile(root / ".git" / "HEAD", std::ios::binary | std::ios::trunc);
    if (!headFile) {
        throw GitError::GitError("HEAD likh nahi sakdi");
    }
    headFile << "ref: " << advertisement.defaultBranch << "\n";

    std::cerr << "HEAD set to " << branch << " (" << hash << ")\n";
}

} // namespace

bool clone(const std::string& url, std::string targetDir) {
    if (targetDir.empty()) {
        targetDir = directoryNameFromUrl(url);
    }

    try {
        std::cout << "Cloning into '" << targetDir << "'...\n";

        GitHttp::Endpoint endpoint = GitHttp::parseUrl(url);
        std::cerr << "Protocol: " << endpoint.scheme << "\n"
                  << "Host: " << endpoint.host << "\n"
                  << "Port: " << endpoint.port << "\n"
                  << "Path: " << endpoint.path << "\n";

        std::filesystem::create_directories(targetDir);
        GitRepository::createSkeleton(targetDir);

        // --- 1. server toh references ---
        std::cerr << "Fetching references...\n";
        RefAdvertisement advertisement =
            parseRefAdvertisement(GitHttp::fetchRefAdvertisement(endpoint));

        for (const auto& ref : advertisement.refs) {
            std::cerr << "Ref: " << ref.name << " -> " << ref.hash << "\n";
        }
        if (advertisement.refs.empty()) {
            throw GitError::GitError("Server toh koi reference nahi mili");
        }

        // --- 2. pack ---
        std::cerr << "Fetching pack file...\n";
        std::vector<std::string> wanted = advertisement.wantedHashes();

        GitPack::Stats stats = GitPack::PackReader(targetDir,
                                                    GitHttp::fetchPack(endpoint, wanted, {}))
                                   .read();
        logPackStats("Pack", stats);

        // REF_DELTA de kuch base is pack vich nahi si - unnu alag maango
        // (Some REF_DELTA bases were not in this pack - ask for them)
        std::vector<std::string> missing = uniqueMissing(stats.missingBases);
        std::vector<std::string> known = wanted;
        for (int round = 0; round < kMaxFetchRounds && !missing.empty(); ++round) {
            std::cerr << "Fetching " << missing.size() << " missing base object(s)...\n";

            std::vector<std::string> haves(known.begin(),
                                           known.begin() + std::min(known.size(), kMaxHaves));
            GitPack::Stats extra =
                GitPack::PackReader(targetDir, GitHttp::fetchPack(endpoint, missing, haves))
                    .read();
            logPackStats("Extra pack", extra);

            known.insert(known.end(), missing.begin(), missing.end());
            missing = uniqueMissing(extra.missingBases);
        }

        if (!missing.empty()) {
            std::ostringstream message;
            message << "Ye object abhi tak nahi mile, te ohnu bina base de apply nahi hona sakda: "
                    << "(these objects still have no base, so their deltas cannot be applied: )";
            for (const auto& hash : missing) {
                message << "\n  " << hash;
            }
            throw GitError::GitError(message.str());
        }

        // --- 3. refs te HEAD ---
        writeAllRefs(targetDir, advertisement);
        writeHead(targetDir, advertisement);

        // --- 4. working tree ---
        std::cerr << "Checking out files...\n";
        GitWorktree::Report report = GitWorktree::checkoutHead(targetDir);
        std::cerr << "Checked out " << report.branch << ": " << report.files << " file(s), "
                  << report.directories << " director(y|ies), " << report.symlinks
                  << " symlink(s)\n";

        std::cout << "Clone complete!\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Clone fail: " << e.what() << "\n";
        return false;
    }
}

} // namespace GitClone
