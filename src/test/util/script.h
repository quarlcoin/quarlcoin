// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_TEST_UTIL_SCRIPT_H
#define QUARLCOIN_TEST_UTIL_SCRIPT_H

#include <crypto/sha256.h>
#include <script/script.h>
#include <script/verify_flags.h>

// A P2WSH program is SHA-256 of the witness script — that is what
// interpreter.cpp hashes when it executes one. These built the program with
// SHA3-256 instead, from the era when the consensus hash was SHA3, so every
// script they produced was one no verifier would ever accept.
static const std::vector<uint8_t> WITNESS_STACK_ELEM_OP_TRUE{uint8_t{OP_TRUE}};
static const CScript P2WSH_OP_TRUE{
    CScript{}
    << OP_0
    << ToByteVector([] {
           uint256 hash;
           CSHA256().Write(WITNESS_STACK_ELEM_OP_TRUE.data(), WITNESS_STACK_ELEM_OP_TRUE.size()).Finalize(hash.begin());
           return hash;
       }())};

static const std::vector<uint8_t> EMPTY{};
static const CScript P2WSH_EMPTY{
    CScript{}
    << OP_0
    << ToByteVector([] {
           uint256 hash;
           CSHA256().Write(EMPTY.data(), EMPTY.size()).Finalize(hash.begin());
           return hash;
       }())};
static const std::vector<std::vector<uint8_t>> P2WSH_EMPTY_TRUE_STACK{{static_cast<uint8_t>(OP_TRUE)}, {}};
static const std::vector<std::vector<uint8_t>> P2WSH_EMPTY_TWO_STACK{{static_cast<uint8_t>(OP_2)}, {}};

/** Flags that are not forbidden by an assert in script validation */
bool IsValidFlagCombination(script_verify_flags flags);

#endif // QUARLCOIN_TEST_UTIL_SCRIPT_H
