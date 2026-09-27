#pragma once

// ==========================================
// COMPRESSION TE DECOMPRESSION DA KAMAAL
// (The magic of compression and decompression)
// ==========================================

#include <cstdint>
#include <string>
#include <vector>

namespace GitCompression {

/**
 * Data nu zlib format vich compress karo
 * (Compress data into zlib format)
 *
 * Git har object nu zlib naal compress karda hai
 * (Git zlib-compresses every loose object)
 */
std::vector<uint8_t> compress(const std::string& data);

/**
 * Compressed data nu wapas kholo
 * (Expand compressed data back)
 */
std::string decompress(const std::vector<uint8_t>& compressed);

/**
 * Convenience overload - string input lainda hai
 * (Convenience overload - takes a string)
 */
std::string decompress(const std::string& compressed);

} // namespace GitCompression
