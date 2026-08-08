// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_INTERFACES_RPC_H
#define QUARLCOIN_INTERFACES_RPC_H

#include <memory>
#include <string>

class UniValue;

namespace node {
struct NodeContext;
} // namespace node

namespace interfaces {
//! Interface giving clients ability to emulate HTTP RPC calls.
class Rpc
{
public:
    virtual ~Rpc() = default;
    virtual UniValue executeRpc(UniValue request, std::string url, std::string user) = 0;
};

//! Return implementation of Rpc interface.
std::unique_ptr<Rpc> MakeRpc(node::NodeContext& node);

} // namespace interfaces

#endif // QUARLCOIN_INTERFACES_RPC_H