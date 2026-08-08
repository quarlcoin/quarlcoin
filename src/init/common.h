// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

//! @file
//! @brief Common init functions shared by bitcoin-node, bitcoin-wallet, etc.

#ifndef QUARLCOIN_INIT_COMMON_H
#define QUARLCOIN_INIT_COMMON_H

#include <util/result.h>

class ArgsManager;

namespace init {
void AddLoggingArgs(ArgsManager& args);
void SetLoggingOptions(const ArgsManager& args);
[[nodiscard]] util::Result<void> SetLoggingCategories(const ArgsManager& args);
[[nodiscard]] util::Result<void> SetLoggingLevel(const ArgsManager& args);
bool StartLogging(const ArgsManager& args);
void LogPackageVersion();
} // namespace init

#endif // QUARLCOIN_INIT_COMMON_H