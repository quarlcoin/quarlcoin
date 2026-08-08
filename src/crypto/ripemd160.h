// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_CRYPTO_RIPEMD160_H
#define QUARLCOIN_CRYPTO_RIPEMD160_H

#include <cstdint>
#include <cstdlib>

/** A hasher class for RIPEMD-160. */
class CRIPEMD160
{
private:
    uint32_t s[5];
    unsigned char buf[64];
    uint64_t bytes{0};

public:
    static const size_t OUTPUT_SIZE = 20;

    CRIPEMD160();
    CRIPEMD160& Write(const unsigned char* data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    CRIPEMD160& Reset();
};

#endif // QUARLCOIN_CRYPTO_RIPEMD160_H
