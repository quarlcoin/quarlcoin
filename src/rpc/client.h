// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_RPC_CLIENT_H
#define QUARLCOIN_RPC_CLIENT_H

#include <string>
#include <string_view>

#include <univalue.h>

/** Convert positional arguments to command-specific RPC representation */
UniValue RPCConvertValues(const std::string& strMethod, const std::vector<std::string>& strParams);

/** Convert named arguments to command-specific RPC representation */
UniValue RPCConvertNamedValues(const std::string& strMethod, const std::vector<std::string>& strParams);

#endif // QUARLCOIN_RPC_CLIENT_H
