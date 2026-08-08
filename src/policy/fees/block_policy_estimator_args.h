// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_POLICY_FEES_BLOCK_POLICY_ESTIMATOR_ARGS_H
#define QUARLCOIN_POLICY_FEES_BLOCK_POLICY_ESTIMATOR_ARGS_H

#include <util/fs.h>

class ArgsManager;

/** @return The fee estimates data file path. */
fs::path FeeestPath(const ArgsManager& argsman);

#endif // QUARLCOIN_POLICY_FEES_BLOCK_POLICY_ESTIMATOR_ARGS_H