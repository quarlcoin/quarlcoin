// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_RANDOMENV_H
#define QUARLCOIN_RANDOMENV_H

#include <crypto/sha512.h>

/** Gather non-cryptographic environment data that changes over time. */
void RandAddDynamicEnv(CSHA512& hasher);

/** Gather non-cryptographic environment data that does not change over time. */
void RandAddStaticEnv(CSHA512& hasher);

#endif // QUARLCOIN_RANDOMENV_H
