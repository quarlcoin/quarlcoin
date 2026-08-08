// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_CONSENSUS_AMOUNT_H
#define QUARLCOIN_CONSENSUS_AMOUNT_H

#include <cstdint>

/** Amount in the smallest unit, 1e-8 QRL (can be negative). */
typedef int64_t CAmount;

/** The number of base units in one QRL. */
static constexpr CAmount COIN = 100000000;

/** No amount larger than this (in res) is valid.
 *
 * Note that this constant is *not* the total money supply, which happens to be
 * less than 21,000,000 QRL for various reasons, but rather a sanity check. As this sanity check is used by consensus-critical
 * validation code, the exact value of the MAX_MONEY constant is consensus
 * critical; in unusual circumstances like a(nother) overflow bug that allowed
 * for the creation of coins out of thin air modification could lead to a fork.
 * */
static constexpr CAmount MAX_MONEY = 21000000 * COIN;
inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }

#endif // QUARLCOIN_CONSENSUS_AMOUNT_H
