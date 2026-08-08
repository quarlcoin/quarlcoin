// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_SUPPORT_CLEANSE_H
#define QUARLCOIN_SUPPORT_CLEANSE_H

#include <cstdlib>

/** Secure overwrite a buffer (possibly containing secret data) with zero-bytes. The write
 * operation will not be optimized out by the compiler. */
void memory_cleanse(void *ptr, size_t len);

#endif // QUARLCOIN_SUPPORT_CLEANSE_H
