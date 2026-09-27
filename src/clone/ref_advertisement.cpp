#include "clone/ref_advertisement.hpp"

#include "clone/pkt_line.hpp"
#include "utils/error.hpp"

#include <algorithm>
#include <map>
#include <sstream>

namespace GitClone {

namespace {

constexpr size_t kHashLength = 40;

bool isHexHash(const std::string& text) {
    if (text.size() < kHashLength) {
        return false;
    }
    for (size_t i = 0; i < kHashLength; ++i) {
        char c = text[i];
        bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!ok) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> splitWords(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream stream(text);
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }
    return words;
}

void trimNewline(std::string& text) {
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
        text.pop_back();
    }
}

// Server nu sab refs yaad rakho, taaki default branch dhoondh sakkein
// (Remember every ref, so we can go looking for the default branch)
void chooseDefaultBranch(RefAdvertisement& advertisement) {
    // 1. Server khud kehda hai: symref=HEAD:refs/heads/trunk
    if (!advertisement.headTarget.empty()) {
        for (const auto& ref : advertisement.refs) {
            if (ref.name == advertisement.headTarget) {
                advertisement.defaultBranch = ref.name;
                return;
            }
        }
    }

    // 2. "main" te "master" - order vich
    for (const char* candidate : {"refs/heads/main", "refs/heads/master"}) {
        for (const auto& ref : advertisement.refs) {
            if (ref.name == candidate) {
                advertisement.defaultBranch = ref.name;
                return;
            }
        }
    }

    // 3. Pehli branch jo mile
    // (The first branch we find)
    for (const auto& ref : advertisement.refs) {
        if (ref.name.rfind("refs/heads/", 0) == 0) {
            advertisement.defaultBranch = ref.name;
            return;
        }
    }
}

} // namespace

std::vector<std::string> RefAdvertisement::wantedHashes() const {
    std::vector<std::string> hashes;
    for (const auto& ref : refs) {
        if (std::find(hashes.begin(), hashes.end(), ref.hash) == hashes.end()) {
            hashes.push_back(ref.hash);
        }
    }
    return hashes;
}

std::string RefAdvertisement::defaultBranchName() const {
    const std::string prefix = "refs/heads/";
    if (defaultBranch.rfind(prefix, 0) == 0) {
        return defaultBranch.substr(prefix.size());
    }
    return defaultBranch;
}

std::string RefAdvertisement::defaultBranchHash() const {
    for (const auto& ref : refs) {
        if (ref.name == defaultBranch) {
            return ref.hash;
        }
    }
    return "";
}

RefAdvertisement parseRefAdvertisement(const std::string& response) {
    RefAdvertisement advertisement;
    bool sawFirstRef = false;

    for (const GitPkt::Line& line : GitPkt::readLines(response)) {
        if (line.isFlush()) {
            continue;
        }

        std::string payload = line.payload;

        // Service announcement: "# service=git-upload-pack"
        if (!payload.empty() && payload[0] == '#') {
            continue;
        }

        // Pehli ref line naal capabilities aundiyan, NUL ke baad
        // (Capabilities ride along on the first ref line, after a NUL)
        std::string capabilities;
        size_t nullPos = payload.find('\0');
        if (nullPos != std::string::npos) {
            capabilities = payload.substr(nullPos + 1);
            payload = payload.substr(0, nullPos);
        }

        trimNewline(payload);
        if (payload.size() <= kHashLength) {
            continue;
        }

        std::string hash = payload.substr(0, kHashLength);
        if (!isHexHash(hash) || payload[kHashLength] != ' ') {
            continue;
        }

        RefLine ref;
        ref.hash = hash;
        ref.name = payload.substr(kHashLength + 1);

        if (!capabilities.empty() && !sawFirstRef) {
            sawFirstRef = true;
            ref.capabilities = splitWords(capabilities);
            for (const auto& capability : ref.capabilities) {
                const std::string prefix = "symref=HEAD:";
                if (capability.rfind(prefix, 0) == 0) {
                    advertisement.headTarget = capability.substr(prefix.size());
                }
            }
        }

        advertisement.refs.push_back(std::move(ref));
    }

    chooseDefaultBranch(advertisement);
    return advertisement;
}

} // namespace GitClone
