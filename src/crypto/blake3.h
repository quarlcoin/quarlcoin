// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_CRYPTO_BLAKE3_H
#define QUARLCOIN_CRYPTO_BLAKE3_H

#include <crypto/blake3/blake3.h>

#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <span>

/** A hasher class for BLAKE3.
 *
 * Sits where CSHA256 sits and offers what CSHA256 offers -- Write, Finalize,
 * Reset, and a chainable Write -- so that hash.h can compose it the way it
 * composes the SHA-256 next door. The vendored C library it calls is in
 * crypto/blake3/; see the UPSTREAM note there for what was imported.
 */
class CBLAKE3
{
private:
    blake3_hasher ctx;

public:
    static constexpr size_t OUTPUT_SIZE{BLAKE3_OUT_LEN};

    CBLAKE3() { blake3_hasher_init(&ctx); }

    CBLAKE3& Write(const unsigned char* data, size_t len)
    {
        blake3_hasher_update(&ctx, data, len);
        return *this;
    }

    CBLAKE3& Write(std::span<const unsigned char> input)
    {
        return Write(input.data(), input.size());
    }

    void Finalize(unsigned char* hash)
    {
        blake3_hasher_finalize(&ctx, hash, OUTPUT_SIZE);
    }

    /** An output of a chosen length, which is what BLAKE3 has and SHA-256 has not.
     *
     *  The extra bytes are the root node read out further, not a second hash and
     *  not a KDF: the first n bytes of any longer output are the n-byte output.
     *  So a 20-byte digest here is a length rather than a truncation, which is
     *  what lets CHash160 be one function instead of SHA-256 then RIPEMD-160. */
    void FinalizeXOF(std::span<unsigned char> output)
    {
        blake3_hasher_finalize(&ctx, output.data(), output.size());
    }

    CBLAKE3& Reset()
    {
        blake3_hasher_init(&ctx);
        return *this;
    }
};

#endif // QUARLCOIN_CRYPTO_BLAKE3_H
