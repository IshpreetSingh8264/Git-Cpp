#include "objects/compression.hpp"

#include "utils/error.hpp"

#include <cstring>

#include <zlib.h>

namespace GitCompression {

namespace {

const size_t kChunkSize = 16384;

} // namespace

std::vector<uint8_t> compress(const std::string& data) {
    z_stream stream;
    std::memset(&stream, 0, sizeof(stream));

    if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        throw GitError::GitError("Compression init fail ho gaya!");
    }

    stream.avail_in = static_cast<uInt>(data.size());
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));

    std::vector<uint8_t> compressed;
    compressed.reserve(data.size());

    std::vector<uint8_t> scratch(kChunkSize);
    int ret;
    do {
        stream.avail_out = static_cast<uInt>(scratch.size());
        stream.next_out = scratch.data();

        ret = deflate(&stream, Z_FINISH);
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&stream);
            throw GitError::GitError("Deflate fail ho gaya!");
        }

        size_t produced = scratch.size() - stream.avail_out;
        compressed.insert(compressed.end(), scratch.begin(), scratch.begin() + produced);
    } while (ret != Z_STREAM_END);

    deflateEnd(&stream);
    return compressed;
}

std::string decompress(const std::vector<uint8_t>& compressed) {
    z_stream stream;
    std::memset(&stream, 0, sizeof(stream));

    // inflateInit khud zlib te gzip dono dekh lainda hai
    // (inflateInit sniffs both the zlib and the gzip wrapper)
    if (inflateInit(&stream) != Z_OK) {
        throw GitError::GitError("Decompression init fail ho gaya!");
    }

    stream.avail_in = static_cast<uInt>(compressed.size());
    stream.next_in = const_cast<Bytef*>(compressed.data());

    std::string decompressed;
    std::vector<uint8_t> scratch(kChunkSize);

    int ret;
    do {
        stream.avail_out = static_cast<uInt>(scratch.size());
        stream.next_out = scratch.data();

        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR ||
            ret == Z_NEED_DICT) {
            inflateEnd(&stream);
            throw GitError::GitError("Inflate fail ho gaya!");
        }

        size_t produced = scratch.size() - stream.avail_out;
        decompressed.append(reinterpret_cast<char*>(scratch.data()), produced);
    } while (ret != Z_STREAM_END);

    inflateEnd(&stream);
    return decompressed;
}

std::string decompress(const std::string& compressed) {
    std::vector<uint8_t> data(compressed.begin(), compressed.end());
    return decompress(data);
}

} // namespace GitCompression
