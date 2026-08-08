/***********************************************************************
 * Copyright (c) 2014 Pieter Wuille                                    *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_HASH_H
#define SECP256K1_HASH_H

#include <stdlib.h>
#include <stdint.h>

#include <crypto/blake3/blake3.h>

/* BLAKE3, not SHA-256, and the name says so.
 *
 * This subtree is a fork of libsecp256k1 from the moment this struct changed.
 * The type kept its shape and its callers -- initialize, write, finalize --
 * because everything above it is written against those, and only the primitive
 * underneath moved. It did not keep its name: a struct called secp256k1_sha256
 * that computes BLAKE3 is the kind of thing a reader trusts without checking,
 * and this is consensus code.
 *
 * Nothing may reach into the state. The old struct exposed s[8] and bytes, and
 * the BIP 340 modules used that to skip a tagged-hash initialisation by writing
 * a precomputed SHA-256 midstate directly. Those eleven places now call
 * secp256k1_hash256_initialize_tagged, which is correct whatever the hash is.
 */
typedef struct {
    blake3_hasher hasher;
} secp256k1_hash256;

static void secp256k1_hash256_initialize(secp256k1_hash256 *hash);
static void secp256k1_hash256_initialize_tagged(secp256k1_hash256 *hash, const unsigned char *tag, size_t taglen);
static void secp256k1_hash256_write(secp256k1_hash256 *hash, const unsigned char *data, size_t size);
static void secp256k1_hash256_finalize(secp256k1_hash256 *hash, unsigned char *out32);
static void secp256k1_hash256_clear(secp256k1_hash256 *hash);

typedef struct {
    secp256k1_hash256 inner, outer;
} secp256k1_hmac_hash256;

static void secp256k1_hmac_hash256_initialize(secp256k1_hmac_hash256 *hash, const unsigned char *key, size_t size);
static void secp256k1_hmac_hash256_write(secp256k1_hmac_hash256 *hash, const unsigned char *data, size_t size);
static void secp256k1_hmac_hash256_finalize(secp256k1_hmac_hash256 *hash, unsigned char *out32);
static void secp256k1_hmac_hash256_clear(secp256k1_hmac_hash256 *hash);

typedef struct {
    unsigned char v[32];
    unsigned char k[32];
    int retry;
} secp256k1_rfc6979_hmac_hash256;

static void secp256k1_rfc6979_hmac_hash256_initialize(secp256k1_rfc6979_hmac_hash256 *rng, const unsigned char *key, size_t keylen);
static void secp256k1_rfc6979_hmac_hash256_generate(secp256k1_rfc6979_hmac_hash256 *rng, unsigned char *out, size_t outlen);
static void secp256k1_rfc6979_hmac_hash256_finalize(secp256k1_rfc6979_hmac_hash256 *rng);
static void secp256k1_rfc6979_hmac_hash256_clear(secp256k1_rfc6979_hmac_hash256 *rng);

#endif /* SECP256K1_HASH_H */
