#pragma once

// ==========================================
// DELTA LAGAANA - PURANE OBJECT TOH NAYA BANANA
// (Applying a delta - rebuilding an object from its base)
// ==========================================

#include <string>

namespace GitDelta {

/**
 * Delta instructions nu base te lagaa ke poora object wapas banaa
 * (Apply delta instructions to a base and rebuild the whole object)
 *
 * Delta da format:
 *   varint  baseSize   - kitna data base vich hai
 *   varint  resultSize - kitna data result vich hona chahida
 *   instructions:
 *     byte & 0x80 -> copy: offset = bits 0-3, size = bits 4-6
 *                    (size 0 matlab 0x10000 bytes)
 *     doosra      -> insert: agle <byte> bytes seedhe copy
 * (Delta format:
 *   varint  baseSize
 *   varint  resultSize
 *   instructions: copy from the base, or insert literal bytes)
 *
 * @param base  - base object da poora content
 * @param delta - delta instructions
 * @return result object da poora content
 * @throws GitError agar delta base te ya result size te na lage
 */
std::string applyDelta(const std::string& base, const std::string& delta);

} // namespace GitDelta
