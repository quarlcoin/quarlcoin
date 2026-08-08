// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_NODE_BLOCKMANAGER_ARGS_H
#define QUARLCOIN_NODE_BLOCKMANAGER_ARGS_H

#include <node/blockstorage.h>
#include <util/result.h>

class ArgsManager;

namespace node {
[[nodiscard]] util::Result<void> ApplyArgsManOptions(const ArgsManager& args, BlockManager::Options& opts);
} // namespace node

#endif // QUARLCOIN_NODE_BLOCKMANAGER_ARGS_H