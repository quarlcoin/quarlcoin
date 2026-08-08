// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_CRYPTO_HKDF_SHA256_32_H
#define QUARLCOIN_CRYPTO_HKDF_SHA256_32_H

#include <cstddef>
#include <string>

/** A rfc5869 HKDF implementation with HMAC_SHA256 and fixed key output length of 32 bytes (L=32) */
class CHKDF_HMAC_SHA256_L32
{
private:
    unsigned char m_prk[32];
    static const size_t OUTPUT_SIZE = 32;

public:
    CHKDF_HMAC_SHA256_L32(const unsigned char* ikm, size_t ikmlen, const std::string& salt);
    void Expand32(const std::string& info, unsigned char hash[OUTPUT_SIZE]);
};

#endif // QUARLCOIN_CRYPTO_HKDF_SHA256_32_H
