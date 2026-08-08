// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_UTIL_SERFLOAT_H
#define QUARLCOIN_UTIL_SERFLOAT_H

#include <cstdint>

/* Encode a double using the IEEE 754 binary64 format. All NaNs are encoded as x86/ARM's
 * positive quiet NaN with payload 0. */
uint64_t EncodeDouble(double f) noexcept;
/* Reverse operation of DecodeDouble. DecodeDouble(EncodeDouble(f))==f unless isnan(f). */
double DecodeDouble(uint64_t v) noexcept;

#endif // QUARLCOIN_UTIL_SERFLOAT_H
