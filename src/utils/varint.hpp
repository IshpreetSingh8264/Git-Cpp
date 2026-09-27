#pragma once

// ==========================================
// VARINT - CHHOTE LIKHE DA NUMBER
// (Varint - numbers written in a compact form)
// ==========================================

#include <cstddef>
#include <cstdint>
#include <string>

namespace GitUtil {

/**
 * Git da varint: har byte de 7 bits, high bit = "halka agla byte aau"
 * (Git's varint: 7 bits per byte, the high bit means "another byte follows")
 *
 * @param data   - bytes toh padhne ne
 *                 (the bytes to read from)
 * @param pos    - kithon padhna hai, pehla value toh 0 hunda hai
 *                 (where to read; incoming value is 0)
 * @param out    - number yahan likhi jaugi
 *                 (the number is written here)
 * @return kitne bytes parhe
 *         (how many bytes were consumed)
 * @throws GitError agar data khatam ho gaya ya number barabar da ho gaya
 */
size_t readVarint(const std::string& data, size_t pos, uint64_t& out);

} // namespace GitUtil
