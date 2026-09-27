#include "clone/delta_applier.hpp"

#include "utils/error.hpp"
#include "utils/varint.hpp"

namespace GitDelta {

namespace {

// Copy instruction de 0 size aaj ki matlab 64 KiB hunda hai
// (A copy instruction's 0 size means 64 KiB)
constexpr uint64_t kZeroSizeMeans64K = 0x10000;

} // namespace

std::string applyDelta(const std::string& base, const std::string& delta) {
    size_t pos = 0;

    uint64_t baseSize = 0;
    pos += GitUtil::readVarint(delta, pos, baseSize);
    if (baseSize != base.size()) {
        throw GitError::GitError("Delta nu bilkul alag base chahida: base " +
                                 std::to_string(base.size()) + " bytes, delta nu " +
                                 std::to_string(baseSize) + " kehta");
    }

    uint64_t resultSize = 0;
    pos += GitUtil::readVarint(delta, pos, resultSize);

    // Dobara lamba result mat banao - 1 GiB de upar te hi error
    // (Never build a runaway result - error out above 1 GiB)
    constexpr uint64_t kMaxResult = 1ULL << 30;
    if (resultSize > kMaxResult) {
        throw GitError::GitError("Delta result taqdeer toh lamba hai: " +
                                 std::to_string(resultSize));
    }

    std::string result;
    result.reserve(static_cast<size_t>(resultSize));

    while (pos < delta.size()) {
        unsigned char instruction = static_cast<unsigned char>(delta[pos++]);

        if ((instruction & 0x80) != 0) {
            // Base vichon chunk copy karo
            // (Copy a chunk out of the base)
            uint64_t copyOffset = 0;
            uint64_t copySize = 0;

            for (int bit = 0; bit <= 3; ++bit) {
                if ((instruction & (1 << bit)) != 0) {
                    if (pos >= delta.size()) {
                        throw GitError::GitError("Delta copy offset beech vich khatam");
                    }
                    copyOffset |= static_cast<uint64_t>(
                                      static_cast<unsigned char>(delta[pos++]))
                                  << (bit * 8);
                }
            }
            for (int bit = 4; bit <= 6; ++bit) {
                if ((instruction & (1 << bit)) != 0) {
                    if (pos >= delta.size()) {
                        throw GitError::GitError("Delta copy size beech vich khatam");
                    }
                    copySize |= static_cast<uint64_t>(
                                    static_cast<unsigned char>(delta[pos++]))
                                << ((bit - 4) * 8);
                }
            }

            if (copySize == 0) {
                copySize = kZeroSizeMeans64K;
            }
            if (copyOffset + copySize > base.size()) {
                throw GitError::GitError("Delta base da bahar copy kar raha hai");
            }

            result.append(base, static_cast<size_t>(copyOffset), static_cast<size_t>(copySize));
        } else {
            // Literal bytes seedhe copy
            // (Literal bytes, copied straight over)
            if (instruction == 0) {
                throw GitError::GitError("Delta vich 0 byte da instruction mila");
            }
            if (pos + instruction > delta.size()) {
                throw GitError::GitError("Delta insert beech vich khatam ho gaya");
            }
            result.append(delta, pos, instruction);
            pos += instruction;
        }
    }

    if (result.size() != resultSize) {
        throw GitError::GitError("Delta result size galat: " +
                                 std::to_string(result.size()) + " bytes banhe, pack nu " +
                                 std::to_string(resultSize) + " kehta");
    }

    return result;
}

} // namespace GitDelta
