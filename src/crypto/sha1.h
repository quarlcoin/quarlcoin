// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_CRYPTO_SHA1_H
#define QUARLCOIN_CRYPTO_SHA1_H

#include <cstdint>
#include <cstdlib>

/** A hasher class for SHA1. */
class CSHA1
{
private:
    uint32_t s[5];
    unsigned char buf[64];
    uint64_t bytes{0};

public:
    static const size_t OUTPUT_SIZE = 20;

    CSHA1();
    CSHA1& Write(const unsigned char* data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    CSHA1& Reset();
};

#endif // QUARLCOIN_CRYPTO_SHA1_H
