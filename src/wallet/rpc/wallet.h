// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_WALLET_RPC_WALLET_H
#define QUARLCOIN_WALLET_RPC_WALLET_H

#include <span.h>

class CRPCCommand;

namespace wallet {
std::span<const CRPCCommand> GetWalletRPCCommands();
} // namespace wallet

#endif // QUARLCOIN_WALLET_RPC_WALLET_H
