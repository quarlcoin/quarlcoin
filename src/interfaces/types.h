// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_INTERFACES_TYPES_H
#define QUARLCOIN_INTERFACES_TYPES_H

#include <uint256.h>

namespace interfaces {

//! Hash/height pair to help track and identify blocks.
struct BlockRef {
    uint256 hash;
    int height = -1;
};

} // namespace interfaces

#endif // QUARLCOIN_INTERFACES_TYPES_H
