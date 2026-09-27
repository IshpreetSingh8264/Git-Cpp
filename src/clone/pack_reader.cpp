#include "clone/pack_reader.hpp"

#include "clone/delta_applier.hpp"
#include "objects/object_store.hpp"
#include "utils/error.hpp"
#include "utils/varint.hpp"

#include <cstring>
#include <vector>

#include <zlib.h>

namespace GitPack {

namespace {

constexpr size_t kHeaderSize = 12;      // "PACK" + version + count
constexpr size_t kRawHashLength = 20;   // binary SHA-1, 20 bytes
constexpr int kMaxDeltaDepth = 100;
constexpr size_t kInflateChunk = 8192;
constexpr size_t kMaxObjectSize = 1ULL << 30;   // 1 GiB per object

std::string hexOf(const unsigned char* raw, size_t length) {
    static const char* digits = "0123456789abcdef";
    std::string hex;
    hex.reserve(length * 2);
    for (size_t i = 0; i < length; ++i) {
        hex.push_back(digits[raw[i] >> 4]);
        hex.push_back(digits[raw[i] & 0x0F]);
    }
    return hex;
}

std::string typeName(PackedType type) {
    switch (type) {
        case PackedType::Commit: return "commit";
        case PackedType::Tree: return "tree";
        case PackedType::Blob: return "blob";
        case PackedType::Tag: return "tag";
        default: return "unknown";
    }
}

uint32_t readUint32(const unsigned char* p) {
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

} // namespace

PackReader::PackReader(std::filesystem::path repoRoot, std::string packBytes)
    : root_(std::move(repoRoot)), pack_(std::move(packBytes)) {}

size_t PackReader::inflateAt(size_t pos, size_t expectedSize, std::string& out) const {
    // Positions are relative to the pack start, which is where "PACK" was found
    const unsigned char* data = reinterpret_cast<const unsigned char*>(pack_.data() + packStart_);

    z_stream stream;
    std::memset(&stream, 0, sizeof(stream));
    if (inflateInit(&stream) != Z_OK) {
        throw GitError::GitError("zlib inflateInit fail ho gaya");
    }

    stream.next_in = const_cast<unsigned char*>(data + pos);
    stream.avail_in = static_cast<uInt>(pack_.size() - packStart_ - pos);

    out.clear();
    out.reserve(expectedSize);
    std::vector<unsigned char> scratch(kInflateChunk);

    int ret;
    do {
        stream.next_out = scratch.data();
        stream.avail_out = static_cast<uInt>(scratch.size());
        ret = inflate(&stream, Z_NO_FLUSH);

        // Z_BUF_ERROR matlab progress hi nahi hua - pack adhoora hai
        // (Z_BUF_ERROR means no progress at all, so the pack is truncated)
        if (ret == Z_BUF_ERROR) {
            inflateEnd(&stream);
            throw GitError::GitError("Pack beech vich khatam ho gaya (adhoora pack)");
        }
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&stream);
            throw GitError::GitError("Pack object inflate nahi hui: zlib code " +
                                     std::to_string(ret));
        }

        size_t produced = scratch.size() - stream.avail_out;
        out.append(reinterpret_cast<char*>(scratch.data()), produced);

        if (out.size() > expectedSize) {
            inflateEnd(&stream);
            throw GitError::GitError("Pack object compressed size te zyada nikal aaya");
        }
    } while (ret != Z_STREAM_END);

    size_t consumed = stream.total_in;
    inflateEnd(&stream);
    return consumed;
}

void PackReader::walk() {
    packStart_ = pack_.find("PACK");
    if (packStart_ == std::string::npos) {
        throw GitError::GitError("Pack signature ('PACK') nahi mili");
    }
    if (pack_.size() - packStart_ < kHeaderSize) {
        throw GitError::GitError("Pack file bahut chhoti hai");
    }

    const unsigned char* data = reinterpret_cast<const unsigned char*>(pack_.data() + packStart_);
    size_t packLength = pack_.size() - packStart_;

    uint32_t version = readUint32(data + 4);
    uint32_t count = readUint32(data + 8);
    if (version != 2 && version != 3) {
        throw GitError::GitError("Pack version samajh nahi aaya: " + std::to_string(version));
    }
    stats_.objects = count;

    size_t pos = kHeaderSize;

    for (uint32_t i = 0; i < count; ++i) {
        if (pos >= packLength) {
            throw GitError::GitError("Pack vich object " + std::to_string(i) +
                                     " di jagah khatam ho gayi");
        }

        Entry entry;
        entry.offset = pos;

        // --- object header: 3 type bits + a variable length size ---
        unsigned char byte = data[pos++];
        PackedType type = static_cast<PackedType>((byte >> 4) & 0x07);
        uint64_t declaredSize = byte & 0x0F;
        int shift = 4;
        while ((byte & 0x80) != 0) {
            if (shift > 63) {
                throw GitError::GitError("Pack object header taqdeer toh lamba hai");
            }
            byte = data[pos++];
            declaredSize |= static_cast<uint64_t>(byte & 0x7F) << shift;
            shift += 7;
        }
        if (declaredSize > kMaxObjectSize) {
            throw GitError::GitError("Pack object taqdeer toh lamba hai: " +
                                     std::to_string(declaredSize));
        }
        entry.type = type;
        entry.declaredSize = static_cast<size_t>(declaredSize);

        if (type == PackedType::OfsDelta) {
            // OFS_DELTA da base ek "negative offset" varint hai jo us object
            // ke shuru teh milta hai. Iko na parhe te padhne da position hi
            // galat ho janda hai.
            // (An OFS_DELTA base is a negative-offset varint relative to this
            //  object's start. Skip it and the read position desyncs.)
            unsigned char ofsByte = data[pos++];
            uint64_t back = ofsByte & 0x7F;
            while ((ofsByte & 0x80) != 0) {
                ofsByte = data[pos++];
                back = ((back + 1) << 7) | (ofsByte & 0x7F);
            }
            if (back == 0 || back > entry.offset) {
                throw GitError::GitError("OFS_DELTA da base offset galat hai: " +
                                         std::to_string(back));
            }
            entry.baseOffset = entry.offset - static_cast<size_t>(back);
        } else if (type == PackedType::RefDelta) {
            // REF_DELTA da base 20 byte binary SHA-1 hai
            if (pos + kRawHashLength > packLength) {
                throw GitError::GitError("REF_DELTA da base hash adhoora hai");
            }
            entry.baseHash = hexOf(data + pos, kRawHashLength);
            pos += kRawHashLength;
        } else if (type != PackedType::Commit && type != PackedType::Tree &&
                   type != PackedType::Blob && type != PackedType::Tag) {
            throw GitError::GitError("Pack vich object type " +
                                     std::to_string(static_cast<int>(type)) + " asaan nahi");
        }

        // --- zlib payload ---
        size_t consumed = inflateAt(pos, entry.declaredSize, entry.data);
        if (entry.data.size() != entry.declaredSize) {
            // Size hi loose object header vich jaanda hai, is liye je data na
            // lage oh corrupt hai - chup na karo
            // (The size is what goes into the loose object header, so data
            //  that does not match means corruption - say so)
            throw GitError::GitError("Pack object da size galat hai: pack kehta " +
                                     std::to_string(entry.declaredSize) + ", mila " +
                                     std::to_string(entry.data.size()));
        }
        pos += consumed;

        byOffset_[entry.offset] = entries_.size();
        entries_.push_back(std::move(entry));
    }
}

PackReader::Materialized PackReader::materialize(size_t offset, int depth) {
    auto memo = resolvedOffset_.find(offset);
    if (memo != resolvedOffset_.end()) {
        return memo->second;
    }

    auto found = byOffset_.find(offset);
    if (found == byOffset_.end()) {
        throw GitError::GitError("Pack vich base object nahi mila, offset " +
                                 std::to_string(offset));
    }
    const Entry& entry = entries_[found->second];

    if (entry.type == PackedType::OfsDelta || entry.type == PackedType::RefDelta) {
        if (depth >= kMaxDeltaDepth) {
            throw GitError::GitError("Delta chain bahut lambi hai");
        }

        Materialized base;
        if (entry.type == PackedType::OfsDelta) {
            // OFS_DELTA da base is pack vich hai te hamesha pichhe hunda hai
            // (An OFS_DELTA base is in this pack and is always earlier)
            base = materialize(entry.baseOffset, depth + 1);
        } else {
            // REF_DELTA da base is pack vich hove te ya objects store vich
            // (A REF_DELTA base is either in this pack or in the object store)
            auto inPack = byHash_.find(entry.baseHash);
            if (inPack != byHash_.end()) {
                base = inPack->second;
            } else if (GitObject::hasObject(root_, entry.baseHash)) {
                GitObject::Object stored =
                    GitObject::splitObject(GitObject::readObject(root_, entry.baseHash));
                base.type = stored.type;
                base.body = std::move(stored.body);
            } else {
                stats_.missingBases.push_back(entry.baseHash);
                throw GitError::GitError("REF_DELTA da base is pack vich nahi: " +
                                         entry.baseHash);
            }
        }

        Materialized result;
        result.type = base.type;
        result.body = GitDelta::applyDelta(base.body, entry.data);
        resolvedOffset_[offset] = result;
        return result;
    }

    Materialized plain;
    plain.type = typeName(entry.type);
    plain.body = entry.data;
    resolvedOffset_[offset] = plain;
    return plain;
}

void PackReader::record(const Materialized& object) {
    // writeObject khud hash bana ke return karda hai, te usde baad hi yaad
    // rakhlo - is order vich sirf pichhe de objects hi mil sakde, je ki
    // Git da vaada hai
    // (writeObject computes the hash and returns it. Remember it right
    //  after: this way only earlier objects are visible, which is exactly
    //  what Git promises.)
    std::string hash = GitObject::writeObject(root_, object.type, object.body);
    byHash_[hash] = object;
    ++stats_.objectsWritten;
}

Stats PackReader::read() {
    walk();

    for (const Entry& entry : entries_) {
        bool isDelta = entry.type == PackedType::OfsDelta || entry.type == PackedType::RefDelta;
        if (isDelta) {
            ++stats_.deltaObjects;
        } else {
            ++stats_.baseObjects;
        }

        try {
            Materialized object = materialize(entry.offset, 0);
            if (isDelta) {
                ++stats_.deltasResolved;
            }
            record(object);
        } catch (const std::exception& e) {
            // Ek object fail hoye te baaki pack chalde rehna chahida, te haan
            // haan galat da record rakhna chahida
            // (One bad object must not kill the rest of the pack, but it has
            //  to be recorded)
            ++stats_.objectsFailed;
            stats_.failures.push_back("offset " + std::to_string(entry.offset) + ": " + e.what());
        }
    }

    return stats_;
}

} // namespace GitPack
