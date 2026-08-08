// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_CRYPTO_SHA512_H
#define QUARLCOIN_CRYPTO_SHA512_H

#include <cstdint>
#include <cstdlib>

/** A hasher class for SHA-512. */
class CSHA512
{
private:
    uint64_t s[8];
    unsigned char buf[128];
    uint64_t bytes{0};

public:
    static constexpr size_t OUTPUT_SIZE = 64;

    CSHA512();
    CSHA512& Write(const unsigned char* data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    CSHA512& Reset();
    uint64_t Size() const { return bytes; }
};

#endif // QUARLCOIN_CRYPTO_SHA512_H
