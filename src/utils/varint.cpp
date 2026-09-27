#include "utils/varint.hpp"

#include "utils/error.hpp"

namespace GitUtil {

size_t readVarint(const std::string& data, size_t pos, uint64_t& out) {
    uint64_t value = 0;
    int shift = 0;
    size_t start = pos;

    while (true) {
        if (pos >= data.size()) {
            throw GitError::GitError("Varint beech vich khatam ho gaya");
        }
        unsigned char byte = static_cast<unsigned char>(data[pos++]);

        // 9 bytes te zyada mat lo, warna 64 bits toh bahar nikal jayenge
        // (Never take more than 9 bytes, 64 bits would overflow)
        if (shift > 63) {
            throw GitError::GitError("Varint taqdeer toh lamba hai");
        }

        value |= static_cast<uint64_t>(byte & 0x7F) << shift;
        shift += 7;

        if ((byte & 0x80) == 0) {
            break;
        }
    }

    out = value;
    return pos - start;
}

} // namespace GitUtil
