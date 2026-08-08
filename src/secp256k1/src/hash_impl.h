/***********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_HASH_IMPL_H
#define SECP256K1_HASH_IMPL_H

#include "hash.h"
#include "util.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* The hash is BLAKE3, taken from crypto/blake3/. What used to stand here was
 * SHA-256 -- the compression function, the message schedule and the padding --
 * about a hundred and fifty lines of it. None of that is needed: BLAKE3 arrives
 * as a library, and this file's job is now to present it under the interface
 * the rest of the subtree already calls.
 *
 * One hash and no second pass. Nothing here ever hashed twice, so there is
 * nothing to remove; the double hashing this chain dropped lives in hash.h at
 * the top of the tree, not in the signature library.
 */

static void secp256k1_hash256_initialize(secp256k1_hash256 *hash) {
    blake3_hasher_init(&hash->hasher);
}

static void secp256k1_hash256_write(secp256k1_hash256 *hash, const unsigned char *data, size_t len) {
    blake3_hasher_update(&hash->hasher, data, len);
}

static void secp256k1_hash256_finalize(secp256k1_hash256 *hash, unsigned char *out32) {
    blake3_hasher_finalize(&hash->hasher, out32, 32);
}

/* Initializes a hash struct and writes the 64-byte string H(tag)||H(tag) into it.
 *
 * The construction is BIP 340's and is kept rather than replaced by BLAKE3's own
 * derive_key mode, for one reason: it is what every caller in this subtree
 * already does, and changing the shape of domain separation at the same time as
 * changing the hash would make two changes indistinguishable if a test failed.
 * Moving to derive_key is a later, separate step -- it removes a hash call per
 * tagged initialisation and nothing else. */
static void secp256k1_hash256_initialize_tagged(secp256k1_hash256 *hash, const unsigned char *tag, size_t taglen) {
    unsigned char buf[32];
    secp256k1_hash256_initialize(hash);
    secp256k1_hash256_write(hash, tag, taglen);
    secp256k1_hash256_finalize(hash, buf);

    secp256k1_hash256_initialize(hash);
    secp256k1_hash256_write(hash, buf, 32);
    secp256k1_hash256_write(hash, buf, 32);
}

static void secp256k1_hash256_clear(secp256k1_hash256 *hash) {
    secp256k1_memclear_explicit(hash, sizeof(*hash));
}

static void secp256k1_hmac_hash256_initialize(secp256k1_hmac_hash256 *hash, const unsigned char *key, size_t keylen) {
    size_t n;
    unsigned char rkey[64];
    if (keylen <= sizeof(rkey)) {
        memcpy(rkey, key, keylen);
        memset(rkey + keylen, 0, sizeof(rkey) - keylen);
    } else {
        secp256k1_hash256 sha256;
        secp256k1_hash256_initialize(&sha256);
        secp256k1_hash256_write(&sha256, key, keylen);
        secp256k1_hash256_finalize(&sha256, rkey);
        memset(rkey + 32, 0, 32);
    }

    secp256k1_hash256_initialize(&hash->outer);
    for (n = 0; n < sizeof(rkey); n++) {
        rkey[n] ^= 0x5c;
    }
    secp256k1_hash256_write(&hash->outer, rkey, sizeof(rkey));

    secp256k1_hash256_initialize(&hash->inner);
    for (n = 0; n < sizeof(rkey); n++) {
        rkey[n] ^= 0x5c ^ 0x36;
    }
    secp256k1_hash256_write(&hash->inner, rkey, sizeof(rkey));
    secp256k1_memclear_explicit(rkey, sizeof(rkey));
}

static void secp256k1_hmac_hash256_write(secp256k1_hmac_hash256 *hash, const unsigned char *data, size_t size) {
    secp256k1_hash256_write(&hash->inner, data, size);
}

static void secp256k1_hmac_hash256_finalize(secp256k1_hmac_hash256 *hash, unsigned char *out32) {
    unsigned char temp[32];
    secp256k1_hash256_finalize(&hash->inner, temp);
    secp256k1_hash256_write(&hash->outer, temp, 32);
    secp256k1_memclear_explicit(temp, sizeof(temp));
    secp256k1_hash256_finalize(&hash->outer, out32);
}

static void secp256k1_hmac_hash256_clear(secp256k1_hmac_hash256 *hash) {
    secp256k1_memclear_explicit(hash, sizeof(*hash));
}

static void secp256k1_rfc6979_hmac_hash256_initialize(secp256k1_rfc6979_hmac_hash256 *rng, const unsigned char *key, size_t keylen) {
    secp256k1_hmac_hash256 hmac;
    static const unsigned char zero[1] = {0x00};
    static const unsigned char one[1] = {0x01};

    memset(rng->v, 0x01, 32); /* RFC6979 3.2.b. */
    memset(rng->k, 0x00, 32); /* RFC6979 3.2.c. */

    /* RFC6979 3.2.d. */
    secp256k1_hmac_hash256_initialize(&hmac, rng->k, 32);
    secp256k1_hmac_hash256_write(&hmac, rng->v, 32);
    secp256k1_hmac_hash256_write(&hmac, zero, 1);
    secp256k1_hmac_hash256_write(&hmac, key, keylen);
    secp256k1_hmac_hash256_finalize(&hmac, rng->k);
    secp256k1_hmac_hash256_initialize(&hmac, rng->k, 32);
    secp256k1_hmac_hash256_write(&hmac, rng->v, 32);
    secp256k1_hmac_hash256_finalize(&hmac, rng->v);

    /* RFC6979 3.2.f. */
    secp256k1_hmac_hash256_initialize(&hmac, rng->k, 32);
    secp256k1_hmac_hash256_write(&hmac, rng->v, 32);
    secp256k1_hmac_hash256_write(&hmac, one, 1);
    secp256k1_hmac_hash256_write(&hmac, key, keylen);
    secp256k1_hmac_hash256_finalize(&hmac, rng->k);
    secp256k1_hmac_hash256_initialize(&hmac, rng->k, 32);
    secp256k1_hmac_hash256_write(&hmac, rng->v, 32);
    secp256k1_hmac_hash256_finalize(&hmac, rng->v);
    rng->retry = 0;
}

static void secp256k1_rfc6979_hmac_hash256_generate(secp256k1_rfc6979_hmac_hash256 *rng, unsigned char *out, size_t outlen) {
    /* RFC6979 3.2.h. */
    static const unsigned char zero[1] = {0x00};
    if (rng->retry) {
        secp256k1_hmac_hash256 hmac;
        secp256k1_hmac_hash256_initialize(&hmac, rng->k, 32);
        secp256k1_hmac_hash256_write(&hmac, rng->v, 32);
        secp256k1_hmac_hash256_write(&hmac, zero, 1);
        secp256k1_hmac_hash256_finalize(&hmac, rng->k);
        secp256k1_hmac_hash256_initialize(&hmac, rng->k, 32);
        secp256k1_hmac_hash256_write(&hmac, rng->v, 32);
        secp256k1_hmac_hash256_finalize(&hmac, rng->v);
    }

    while (outlen > 0) {
        secp256k1_hmac_hash256 hmac;
        size_t now = outlen;
        secp256k1_hmac_hash256_initialize(&hmac, rng->k, 32);
        secp256k1_hmac_hash256_write(&hmac, rng->v, 32);
        secp256k1_hmac_hash256_finalize(&hmac, rng->v);
        if (now > 32) {
            now = 32;
        }
        memcpy(out, rng->v, now);
        out += now;
        outlen -= now;
    }

    rng->retry = 1;
}

static void secp256k1_rfc6979_hmac_hash256_finalize(secp256k1_rfc6979_hmac_hash256 *rng) {
    (void) rng;
}

static void secp256k1_rfc6979_hmac_hash256_clear(secp256k1_rfc6979_hmac_hash256 *rng) {
    secp256k1_memclear_explicit(rng, sizeof(*rng));
}

#undef Round
#undef sigma1
#undef sigma0
#undef Sigma1
#undef Sigma0
#undef Maj
#undef Ch

#endif /* SECP256K1_HASH_IMPL_H */
