// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_HASH_H
#define QUARLCOIN_HASH_H

#include <attributes.h>
#include <crypto/blake3.h>
#include <crypto/common.h>
#include <crypto/ripemd160.h>
#include <crypto/sha256.h>
#include <prevector.h>
#include <serialize.h>
#include <span.h>
#include <uint256.h>

#include <string>
#include <vector>

typedef uint256 ChainCode;

/** A hasher class for Quarlcoin's 256-bit hash: BLAKE3, once.
 *
 * Bitcoin hashes twice here, and the second pass is not belt and braces -- it
 * is a repair. SHA-256 is a Merkle-Damgard construction and leaks a length
 * extension: given H(m) and the length of m, anyone can compute
 * H(m || padding || suffix) without ever seeing m. Hashing the digest again
 * closes that, at the cost of doing the work twice.
 *
 * BLAKE3 is a tree over a keyed permutation with a finalised root, so H(m)
 * says nothing about H(m || anything). There is nothing for a second pass to
 * repair, and it would cost half the speed of every txid, every merkle node
 * and every header this chain ever checks.
 *
 * The class keeps Core's name and Core's shape on purpose: everything in this
 * tree that hashes was written against Write/Finalize/Reset, and swapping the
 * primitive underneath is a change here rather than a change at eight hundred
 * call sites.
 */
class CHash256 {
private:
    CBLAKE3 blake;
public:
    static const size_t OUTPUT_SIZE = CBLAKE3::OUTPUT_SIZE;

    void Finalize(std::span<unsigned char> output) {
        assert(output.size() == OUTPUT_SIZE);
        blake.Finalize(output.data());
    }

    CHash256& Write(std::span<const unsigned char> input) {
        blake.Write(input);
        return *this;
    }

    CHash256& Reset() {
        blake.Reset();
        return *this;
    }
};

/** A hasher class for Quarlcoin's 160-bit hash: BLAKE3, read out to 20 bytes.
 *
 * Bitcoin composes two functions here, SHA-256 then RIPEMD-160, because it
 * needed 160 bits and SHA-256 gives 256. BLAKE3's output is a stream read off
 * the root node, so 20 bytes is a length rather than a truncation -- and the
 * first 20 bytes of a 32-byte BLAKE3 output are exactly these 20 bytes, which
 * is a property the SHA-256-then-RIPEMD-160 composition never had.
 *
 * That removes RIPEMD-160 from address derivation entirely. It stays in the
 * tree for OP_RIPEMD160, which is a script opcode and not an address.
 */
class CHash160 {
private:
    CBLAKE3 blake;
public:
    static const size_t OUTPUT_SIZE = 20;

    void Finalize(std::span<unsigned char> output) {
        assert(output.size() == OUTPUT_SIZE);
        blake.FinalizeXOF(output);
    }

    CHash160& Write(std::span<const unsigned char> input) {
        blake.Write(input);
        return *this;
    }

    CHash160& Reset() {
        blake.Reset();
        return *this;
    }
};

/** Compute the 256-bit hash of an object. */
template<typename T>
inline uint256 Hash(const T& in1)
{
    uint256 result;
    CHash256().Write(MakeUCharSpan(in1)).Finalize(result);
    return result;
}

/** Compute the 256-bit hash of the concatenation of two objects. */
template<typename T1, typename T2>
inline uint256 Hash(const T1& in1, const T2& in2) {
    uint256 result;
    CHash256().Write(MakeUCharSpan(in1)).Write(MakeUCharSpan(in2)).Finalize(result);
    return result;
}

/** Compute the 160-bit hash an object. */
template<typename T1>
inline uint160 Hash160(const T1& in1)
{
    uint160 result;
    CHash160().Write(MakeUCharSpan(in1)).Finalize(result);
    return result;
}

/** A writer stream (for serialization) that computes a 256-bit hash. */
class HashWriter
{
private:
    CBLAKE3 ctx;

public:
    void write(std::span<const std::byte> src)
    {
        ctx.Write(UCharCast(src.data()), src.size());
    }

    /** The BLAKE3 hash of everything written here. Invalidates this object. */
    uint256 GetHash() {
        uint256 result;
        ctx.Finalize(result.begin());
        return result;
    }

    /**
     * Returns the first 64 bits from the resulting hash.
     */
    inline uint64_t GetCheapHash() {
        uint256 result = GetHash();
        return ReadLE64(result.begin());
    }

    template <typename T>
    HashWriter& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return *this;
    }
};

/** A writer stream that computes SHA-256, for BIP 340 and BIP 341 only.
 *
 * Everything this chain hashes for itself goes through HashWriter and is
 * BLAKE3. What comes through here is the taproot machinery -- tagged hashes,
 * TapLeaf, TapBranch, the taproot tweak -- where the hash function is fixed by
 * the signature scheme rather than by this chain. libsecp256k1 computes the
 * Schnorr challenge with SHA-256 inside itself; substituting BLAKE3 in the
 * functions around it would not change that, it would only put two hash
 * functions where the specification has one.
 *
 * A separate class rather than a second digest inside HashWriter: keeping both
 * in one object would have every txid, every merkle node and every sighash pay
 * for a SHA-256 that thirteen call sites use and nothing else ever reads.
 */
class SHA256Writer
{
private:
    CSHA256 ctx;

public:
    void write(std::span<const std::byte> src)
    {
        ctx.Write(UCharCast(src.data()), src.size());
    }

    /** Invalidates this object. */
    uint256 GetSHA256() {
        uint256 result;
        ctx.Finalize(result.begin());
        return result;
    }

    template <typename T>
    SHA256Writer& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return *this;
    }
};

/** Reads data from an underlying stream, while hashing the read data. */
template <typename Source>
class HashVerifier : public HashWriter
{
private:
    Source& m_source;

public:
    explicit HashVerifier(Source& source LIFETIMEBOUND) : m_source{source} {}

    void read(std::span<std::byte> dst)
    {
        m_source.read(dst);
        this->write(dst);
    }

    void ignore(size_t num_bytes)
    {
        std::byte data[1024];
        while (num_bytes > 0) {
            size_t now = std::min<size_t>(num_bytes, 1024);
            read({data, now});
            num_bytes -= now;
        }
    }

    template <typename T>
    HashVerifier<Source>& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return *this;
    }
};

/** Writes data to an underlying source stream, while hashing the written data. */
template <typename Source>
class HashedSourceWriter : public HashWriter
{
private:
    Source& m_source;

public:
    explicit HashedSourceWriter(Source& source LIFETIMEBOUND) : HashWriter{}, m_source{source} {}

    void write(std::span<const std::byte> src)
    {
        m_source.write(src);
        HashWriter::write(src);
    }

    template <typename T>
    HashedSourceWriter& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return *this;
    }
};

/** Single-SHA256 a 32-byte input (represented as uint256). */
[[nodiscard]] uint256 SHA256Uint256(const uint256& input);

unsigned int MurmurHash3(unsigned int nHashSeed, std::span<const unsigned char> vDataToHash);

void BIP32Hash(const ChainCode &chainCode, unsigned int nChild, unsigned char header, const unsigned char data[32], unsigned char output[64]);

/** Return a SHA256Writer primed for tagged hashes (as specified in BIP 340).
 *
 * The returned object will have SHA256(tag) written to it twice (= 64 bytes).
 * A tagged hash can be computed by feeding the message into this object, and
 * then calling SHA256Writer::GetSHA256().
 */
SHA256Writer TaggedHash(const std::string& tag);

/** Compute the 160-bit RIPEMD-160 hash of an array. */
inline uint160 RIPEMD160(std::span<const unsigned char> data)
{
    uint160 result;
    CRIPEMD160().Write(data.data(), data.size()).Finalize(result.begin());
    return result;
}

#endif // QUARLCOIN_HASH_H
