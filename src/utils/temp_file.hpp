#pragma once

// ==========================================
// GADD DA THAAL - SAFE TEMPORARY FILE
// (Scratch pot - a safe temporary file)
// ==========================================

#include <filesystem>
#include <string>

namespace GitUtil {

/**
 * File jo apne aap delete ho jandi hai
 * (A file that deletes itself)
 *
 * mkstemp unpredictable naam bananda hai te fd turant band kar
 * dinda hai, is nu koi doosra process guess nahi kar sakta.
 * Destructor har raaste te file mita dinda hai - error aaeya te
 * bhi, exception aaeya te bhi.
 * (mkstemp creates an unpredictable name and we close the fd
 *  immediately, so nobody can guess it. The destructor unlinks
 *  the file on every path - success, error, or exception.)
 *
 * Copy nahi hunda - move hunda hai
 * (Not copyable - movable)
 */
class TempFile {
public:
    // suffix: ".pack" ya ".txt" jaisa extension
    // (suffix: an extension like ".pack" or ".txt")
    explicit TempFile(const std::string& suffix = "");

    ~TempFile();

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    TempFile(TempFile&& other) noexcept;
    TempFile& operator=(TempFile&& other) noexcept;

    const std::filesystem::path& path() const noexcept { return path_; }

    // File vich likh ke fd band kar dinda hai
    // (Writes the bytes and closes the fd)
    void write(const std::string& bytes);

    // File da poora content padh ke return karda hai
    // (Reads the whole file and returns it)
    std::string read() const;

    // Jaaan dein - file delete nahi hogi (testing vich kaam aunda)
    // (Let it live - the file is not deleted. Handy in tests.)
    std::filesystem::path release() noexcept;

private:
    std::filesystem::path path_;
};

} // namespace GitUtil
