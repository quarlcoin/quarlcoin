// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_UTIL_CHAINTYPE_H
#define QUARLCOIN_UTIL_CHAINTYPE_H

#include <optional>
#include <string>
#include <string_view>

enum class ChainType {
    MAIN,
    TESTNET,
    REGTEST,
};

std::string ChainTypeToString(ChainType chain);

std::optional<ChainType> ChainTypeFromString(std::string_view chain);

#endif // QUARLCOIN_UTIL_CHAINTYPE_H
