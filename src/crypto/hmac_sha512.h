// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_CRYPTO_HMAC_SHA512_H
#define QUARLCOIN_CRYPTO_HMAC_SHA512_H

#include <crypto/sha512.h>

#include <cstddef>

/** A hasher class for HMAC-SHA-512. */
class CHMAC_SHA512
{
private:
    CSHA512 outer;
    CSHA512 inner;

public:
    static const size_t OUTPUT_SIZE = 64;

    CHMAC_SHA512(const unsigned char* key, size_t keylen);
    CHMAC_SHA512& Write(const unsigned char* data, size_t len)
    {
        inner.Write(data, len);
        return *this;
    }
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
};

#endif // QUARLCOIN_CRYPTO_HMAC_SHA512_H
