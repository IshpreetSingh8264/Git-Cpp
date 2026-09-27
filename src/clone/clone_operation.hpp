#pragma once

// ==========================================
// CLONE KARNE DA SYSTEM
// (The clone system)
// ==========================================

#include <string>

namespace GitClone {

/**
 * Remote repository nu local vich leke aa
 * (Bring a remote repository down to local)
 *
 * @param url        - repository URL
 * @param targetDir  - jithon rakhna hai, khaali dasso te URL toh naam aayega
 *                     (where to put it; empty means take the name from the URL)
 * @return true te sab theek, false te kuch galat
 *         (true when it all worked, false when something did not)
 */
bool clone(const std::string& url, std::string targetDir = "");

} // namespace GitClone
