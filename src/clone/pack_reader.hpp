#pragma once

// ==========================================
// PACK FILE PADHNA - DELTA SAATH
// (Reading a pack file - deltas included)
// ==========================================

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace GitPack {

/**
 * Pack object de type
 * (An object's type inside a pack)
 *
 * 1 commit, 2 tree, 3 blob, 4 tag, 6 OFS_DELTA, 7 REF_DELTA
 */
enum class PackedType : uint8_t {
    Commit = 1,
    Tree = 2,
    Blob = 3,
    Tag = 4,
    OfsDelta = 6,
    RefDelta = 7,
};

/**
 * Pack vich da ek object, jado tak oh inflate nahi hoya
 * (One object inside a pack, before it is inflated)
 */
struct Entry {
    size_t offset = 0;          // header da jagah, pack vich
    PackedType type = PackedType::Blob;
    size_t declaredSize = 0;    // pack header da size field
    size_t baseOffset = 0;      // OFS_DELTA lai base da pack offset
    std::string baseHash;       // REF_DELTA lai base da 40 character hash
    std::string data;           // inflate hoya hua payload
};

/**
 * Poore pack da hisaab
 * (The pack's tally)
 */
struct Stats {
    size_t objects = 0;           // pack header de hisaab vich kitne
    size_t baseObjects = 0;       // raw (non-delta) objects
    size_t deltaObjects = 0;      // OFS_DELTA + REF_DELTA
    size_t deltasResolved = 0;    // jinke base mil gaye
    size_t objectsWritten = 0;    // jo loose objects bane
    size_t objectsFailed = 0;     // jo samajh nahi aaye
    std::vector<std::string> failures;      // kya galat hoya, kyun
    std::vector<std::string> missingBases;  // je base hash chahiye si

    /**
     * Kuch te serious galat hoya?
     * (Did anything serious go wrong?)
     */
    bool ok() const { return objectsFailed == 0 && missingBases.empty(); }
};

/**
 * Pack file padh ke usde objects loose object banawan da kamaal
 * (Read a pack file and turn its objects into loose objects)
 *
 * Pehle poora pack walk karo, phir har object nu materialize karo.
 * OFS_DELTA da base hamesha pichhe hunda hai, is liye order vich
 * chalna kaam karda hai. REF_DELTA da base ya te is pack vich hunda
 * hai ya te objects store vich - dono try karo, dono na mile te hash
 * report kar do taaki caller ohnu alag maang sake.
 * (We walk the whole pack first, then materialize every object. An
 *  OFS_DELTA base is always earlier, which is why going in order works.
 *  A REF_DELTA base is either in this pack or in the object store - we
 *  try both, and report any hash that is still missing so the caller can
 *  ask for it separately.)
 */
class PackReader {
public:
    PackReader(std::filesystem::path repoRoot, std::string packBytes);

    /**
     * Pack padho te objects likho
     * (Read the pack and write the objects)
     */
    Stats read();

private:
    struct Materialized {
        std::string type;
        std::string body;
    };

    void walk();                                     // index + inflate every entry
    size_t inflateAt(size_t pos, size_t expectedSize, std::string& out) const;
    Materialized materialize(size_t offset, int depth);
    void record(const Materialized& object);

    std::filesystem::path root_;
    std::string pack_;
    size_t packStart_ = 0;
    std::vector<Entry> entries_;
    std::map<size_t, size_t> byOffset_;              // pack offset -> index in entries_
    std::map<size_t, Materialized> resolvedOffset_;  // memo, so chains are not redone
    std::map<std::string, Materialized> byHash_;
    Stats stats_;
};

} // namespace GitPack
