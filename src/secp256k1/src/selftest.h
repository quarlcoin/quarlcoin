/***********************************************************************
 * Copyright (c) 2020 Pieter Wuille                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_SELFTEST_H
#define SECP256K1_SELFTEST_H

#include "hash.h"

#include <string.h>

/* A known answer for the hash this library actually uses.
 *
 * The input is upstream's, kept so the two can be compared by eye; the expected
 * output is BLAKE3's and was recomputed, not translated. This check exists to
 * catch a build in which the hash is miscompiled or mislinked -- exactly the
 * failure that a swapped primitive makes possible -- so it is the one place
 * where a stale constant would be worse than no check at all. */
static int secp256k1_selftest_hash256(void) {
    static const char *input63 = "For this sample, this 63-byte string will be used as input data";
    static const unsigned char output32[32] = {
        0x90, 0x1c, 0x39, 0x2e, 0xb9, 0x59, 0x7a, 0x4d, 0x82, 0xc0, 0xeb, 0x16, 0x9d, 0x4d, 0xe0, 0x8a,
        0x19, 0x4e, 0x8e, 0x3a, 0x6e, 0x4c, 0xa9, 0x2d, 0xb6, 0x41, 0x9f, 0xe2, 0x6d, 0xca, 0x17, 0x3c,
    };
    unsigned char out[32];
    secp256k1_hash256 hasher;
    secp256k1_hash256_initialize(&hasher);
    secp256k1_hash256_write(&hasher, (const unsigned char*)input63, 63);
    secp256k1_hash256_finalize(&hasher, out);
    return secp256k1_memcmp_var(out, output32, 32) == 0;
}

static int secp256k1_selftest_passes(void) {
    return secp256k1_selftest_hash256();
}

#endif /* SECP256K1_SELFTEST_H */
