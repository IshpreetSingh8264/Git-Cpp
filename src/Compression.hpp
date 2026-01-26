#pragma once

#include <string>
#include <vector>
#include <zlib.h>
#include <stdexcept>
#include <cstring>

// ==========================================
// COMPRESSION TE DECOMPRESSION DA KAMAAL
// (The magic of compression and decompression)
// ==========================================

namespace GitCompression {

/**
 * Oi puttar, eh function data nu compress karda hai
 * (Hey kiddo, this function compresses data)
 * 
 * Git vich saari cheezaan compress hondiyan ne - thoda space bachao te
 * (Everything in Git gets compressed - save some space, you know)
 * 
 * @param data - Raw data jo compress karna hai (Raw data to compress)
 * @return Compressed data as a vector of bytes
 */
inline std::vector<uint8_t> compress(const std::string& data) {
    // Pehle zlib stream setup karo, bilkul bike repair karni ho
    // (First setup zlib stream, just like fixing a bike)
    z_stream stream;
    std::memset(&stream, 0, sizeof(stream));
    
    // Default compression level - na zyada tight, na dhila
    // (Default compression level - not too tight, not too loose)
    if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        throw std::runtime_error("Arre bapu! Compression init fail ho gaya!"
                               "\n(Oh father! Compression init failed!)");
    }
    
    // Input data set karo
    // (Set the input data)
    stream.avail_in = data.size();
    stream.next_in = reinterpret_cast<uint8_t*>(const_cast<char*>(data.data()));
    
    // Output buffer - jitthe compressed data store hoga
    // (Output buffer - where compressed data will be stored)
    std::vector<uint8_t> compressed;
    compressed.reserve(data.size()); // Shuru vich thodi jagah reserve kar lo
                                     // (Reserve some space initially)
    
    // Chunk chunk karke compress karo, lassi vaang
    // (Compress chunk by chunk, like churning lassi)
    const size_t CHUNK_SIZE = 16384; // 16KB chunks - perfect size, trust me
    uint8_t temp_buffer[CHUNK_SIZE];
    
    int ret;
    do {
        stream.avail_out = CHUNK_SIZE;
        stream.next_out = temp_buffer;
        
        // Dabao dabao, sab kucch compress karo!
        // (Press press, compress everything!)
        ret = deflate(&stream, Z_FINISH);
        
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&stream);
            throw std::runtime_error("Haww! Deflate fail ho gaya!"
                                   "\n(Oh no! Deflate failed!)");
        }
        
        // Jo compress ho gaya, usse result vich daal do
        // (Put what got compressed into the result)
        size_t have = CHUNK_SIZE - stream.avail_out;
        compressed.insert(compressed.end(), temp_buffer, temp_buffer + have);
        
    } while (ret != Z_STREAM_END);
    
    // Cleanup kar lo, ghar saaf rakho
    // (Clean up, keep the house clean)
    deflateEnd(&stream);
    
    return compressed;
}

/**
 * Haan ji, eh decompress karda hai
 * (Yes sir, this one decompresses)
 * 
 * Git objects nu padhna hai? Pehle decompress karo!
 * (Want to read Git objects? First decompress!)
 * 
 * @param compressed - Compressed data jo expand karna hai (Compressed data to expand)
 * @return Decompressed data as a string
 */
inline std::string decompress(const std::vector<uint8_t>& compressed) {
    // Stream setup - ek vaari phir
    // (Stream setup - one more time)
    z_stream stream;
    std::memset(&stream, 0, sizeof(stream));
    
    // Automatic header detection - smart banda, khud samajh jaanda
    // (Automatic header detection - smart guy, understands on its own)
    if (inflateInit(&stream) != Z_OK) {
        throw std::runtime_error("Arre yaar! Decompression init fail!"
                               "\n(Oh buddy! Decompression init failed!)");
    }
    
    // Input set karo
    // (Set the input)
    stream.avail_in = compressed.size();
    stream.next_in = const_cast<uint8_t*>(compressed.data());
    
    // Output string jitthe sab kucch expand hoga
    // (Output string where everything will expand)
    std::string decompressed;
    
    // Phudko te expand karo!
    // (Puff up and expand!)
    const size_t CHUNK_SIZE = 16384;
    char temp_buffer[CHUNK_SIZE];
    
    int ret;
    do {
        stream.avail_out = CHUNK_SIZE;
        stream.next_out = reinterpret_cast<uint8_t*>(temp_buffer);
        
        // Inflate karo - balloon vaang!
        // (Inflate it - like a balloon!)
        ret = inflate(&stream, Z_NO_FLUSH);
        
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&stream);
            throw std::runtime_error("Tauba tauba! Inflate fail ho gaya!"
                                   "\n(Oh my! Inflate failed!)");
        }
        
        // Decompressed data nu result vich daal do
        // (Put decompressed data into result)
        size_t have = CHUNK_SIZE - stream.avail_out;
        decompressed.append(temp_buffer, have);
        
    } while (ret != Z_STREAM_END);
    
    // Saaf safai
    // (Cleanup)
    inflateEnd(&stream);
    
    return decompressed;
}

/**
 * Overload for string input - kyunki convenience zaroori hai
 * (Overload for string input - because convenience is important)
 */
inline std::string decompress(const std::string& compressed) {
    std::vector<uint8_t> data(compressed.begin(), compressed.end());
    return decompress(data);
}

} // namespace GitCompression
