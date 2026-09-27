#include "clone/pkt_line.hpp"

namespace GitPkt {

namespace {

// 4 hex digits nu number vich badlo
// (Turn 4 hex digits into a number)
bool parseLength(const std::string& text, long& out) {
    if (text.size() != 4) {
        return false;
    }
    long value = 0;
    for (char c : text) {
        int digit;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            return false;
        }
        value = value * 16 + digit;
    }
    out = value;
    return true;
}

} // namespace

std::vector<Line> readLines(const std::string& data) {
    std::vector<Line> lines;
    size_t pos = 0;

    while (pos + 4 <= data.size()) {
        long length = 0;
        if (!parseLength(data.substr(pos, 4), length)) {
            break;
        }

        if (length == 0) {
            pos += 4;
            lines.push_back(Line{"", true});
            continue;
        }
        if (length < 4) {
            // 0001 delim packet. Response da baaki hissa refs nahi hunda
            // (A 0001 delim packet; whatever follows is not part of the refs)
            break;
        }
        if (pos + static_cast<size_t>(length) > data.size()) {
            // Adhoori line - cut hoyi response
            // (A partial line - the response was cut)
            break;
        }

        // Length vich 4 length bytes bhi ginye hunde ne, is liye eho
        // poore line di lambi hai - pos sirf ik hi baar aage badhna hai
        // (The length counts its own 4 bytes, so it is the whole line
        //  length and the position moves forward exactly once)
        lines.push_back(Line{data.substr(pos + 4, static_cast<size_t>(length) - 4), false});
        pos += static_cast<size_t>(length);
    }

    return lines;
}

} // namespace GitPkt
