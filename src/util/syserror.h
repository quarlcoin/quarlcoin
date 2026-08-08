// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_UTIL_SYSERROR_H
#define QUARLCOIN_UTIL_SYSERROR_H

#include <string>

/** Return system error string from errno value. Use this instead of
 * std::strerror, which is not thread-safe. For network errors use
 * NetworkErrorString from sock.h instead.
 */
std::string SysErrorString(int err);

#if defined(WIN32)
std::string Win32ErrorString(int err);
#endif

#endif // QUARLCOIN_UTIL_SYSERROR_H
