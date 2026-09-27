#pragma once

// ==========================================
// PKT-LINE - GIT DA LINE FORMAT
// (pkt-line - Git's line format)
// ==========================================

#include <string>
#include <vector>

namespace GitPkt {

/**
 * Ek pkt-line
 * (One pkt-line)
 */
struct Line {
    std::string payload;  // 4 byte length heer ke baad da content
    bool flush = false;   // "0000" packet, yaani section khatam

    /**
     * Kya eh "0000" flush packet hai?
     * (Is this a "0000" flush packet?)
     */
    bool isFlush() const { return flush; }
};

/**
 * Response vichon saare pkt-line nikaal lo
 * (Pull every pkt-line out of a response)
 *
 * Format: 4 hex digits da length, phir utna hi data. "0000" flush
 * packet hai te "0001" delim packet hai - dono nu chhod dein haan.
 * (Format: a 4 hex digit length, then that many bytes. "0000" is a
 *  flush packet and "0001" a delim packet - we skip both.)
 *
 * Adhoori ya bigadi line nu chup chaap chhod dein haan, kyunki remote
 * ne response cut kar dita hunda
 * (A truncated or malformed line is skipped quietly, because the far
 *  end may have cut the response short)
 */
std::vector<Line> readLines(const std::string& data);

} // namespace GitPkt
