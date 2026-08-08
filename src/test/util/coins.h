// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_TEST_UTIL_COINS_H
#define QUARLCOIN_TEST_UTIL_COINS_H

#include <primitives/transaction.h>

class CCoinsViewCache;
class FastRandomContext;

/**
 * Create a Coin with DynamicMemoryUsage of 80 bytes and add it to the given view.
 * @param[in,out] coins_view  The coins view cache to add the new coin to.
 * @returns the COutPoint of the created coin.
 */
COutPoint AddTestCoin(FastRandomContext& rng, CCoinsViewCache& coins_view);

#endif // QUARLCOIN_TEST_UTIL_COINS_H
