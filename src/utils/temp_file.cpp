#include "utils/temp_file.hpp"

#include "utils/error.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <system_error>
#include <vector>

#include <unistd.h>

namespace GitUtil {

namespace {

// File kithon banani hai - TMPDIR, warna /tmp
// (Where to create the file - TMPDIR, else /tmp)
std::string tempDir() {
    const char* dir = std::getenv("TMPDIR");
    if (dir != nullptr && dir[0] != '\0') {
        return dir;
    }
    return "/tmp";
}

} // namespace

TempFile::TempFile(const std::string& suffix) {
    // mkstemp cheez nu XXXXXX chahida - suffix laage rehna chahida nahi
    // (mkstemp wants an XXXXXX template - a suffix after it is not allowed)
    std::string tmpl = tempDir() + "/git-" + std::to_string(::getpid()) + "-XXXXXX";
    std::vector<char> buffer(tmpl.begin(), tmpl.end());
    buffer.push_back('\0');

    int fd = ::mkstemp(buffer.data());
    if (fd < 0) {
        throw GitError::GitError("Temporary file nahi ban sakdi: " + std::string(std::strerror(errno)));
    }

    // mkstemp 0600 permissions de dinda hai, te fd band kar dinde haan
    // (mkstemp already gives us 0600; close the fd, the name is all we want)
    ::close(fd);

    path_ = std::filesystem::path(buffer.data());
    if (!suffix.empty()) {
        // Extension chahida si toh rename kar lo
        // (A suffix was requested, so rename)
        std::filesystem::path renamed = path_;
        renamed += suffix;
        std::error_code ec;
        std::filesystem::rename(path_, renamed, ec);
        if (!ec) {
            path_ = renamed;
        }
    }
}

TempFile::~TempFile() {
    if (!path_.empty()) {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }
}

TempFile::TempFile(TempFile&& other) noexcept : path_(std::move(other.path_)) {
    other.path_.clear();
}

TempFile& TempFile::operator=(TempFile&& other) noexcept {
    if (this != &other) {
        if (!path_.empty()) {
            std::error_code ec;
            std::filesystem::remove(path_, ec);
        }
        path_ = std::move(other.path_);
        other.path_.clear();
    }
    return *this;
}

void TempFile::write(const std::string& bytes) {
    std::ofstream out(path_, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw GitError::GitError("Temporary file likh nahi sakdi: " + path_.string());
    }
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!out) {
        throw GitError::GitError("Temporary file likh nahi sakdi: " + path_.string());
    }
}

std::string TempFile::read() const {
    std::ifstream in(path_, std::ios::binary);
    if (!in) {
        throw GitError::GitError("Temporary file nahi khul sakdi: " + path_.string());
    }
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::filesystem::path TempFile::release() noexcept {
    std::filesystem::path out = path_;
    path_.clear();
    return out;
}

} // namespace GitUtil
