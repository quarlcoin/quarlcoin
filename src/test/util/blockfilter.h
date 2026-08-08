// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_TEST_UTIL_BLOCKFILTER_H
#define QUARLCOIN_TEST_UTIL_BLOCKFILTER_H

#include <blockfilter.h>

class CBlockIndex;
namespace node {
class BlockManager;
}

bool ComputeFilter(BlockFilterType filter_type, const CBlockIndex& block_index, BlockFilter& filter, const node::BlockManager& blockman);

#endif // QUARLCOIN_TEST_UTIL_BLOCKFILTER_H
