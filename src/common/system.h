// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_COMMON_SYSTEM_H
#define QUARLCOIN_COMMON_SYSTEM_H

#include <quarlcoin-build-config.h> // IWYU pragma: keep
#include <util/time.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

/// Monotonic uptime (not affected by system time changes).
SteadyClock::duration GetUptime();

void SetupEnvironment();
[[nodiscard]] bool SetupNetworking();
#ifndef WIN32
std::string ShellEscape(const std::string& arg);
#endif
#if HAVE_SYSTEM
void runCommand(const std::string& strCommand);
#endif

/**
 * Return the number of cores available on the current system.
 * @note This does count virtual cores, such as those provided by HyperThreading.
 */
int GetNumCores();

/**
 * Return the total RAM available on the current system, if detectable.
 */
std::optional<size_t> GetTotalRAM();

#endif // QUARLCOIN_COMMON_SYSTEM_H
