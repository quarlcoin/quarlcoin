// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_RPC_MEMPOOL_H
#define QUARLCOIN_RPC_MEMPOOL_H

class CTxMemPool;
class UniValue;

/** Mempool information to JSON */
UniValue MempoolInfoToJSON(const CTxMemPool& pool);

/** Mempool to JSON */
UniValue MempoolToJSON(const CTxMemPool& pool, bool verbose = false, bool include_mempool_sequence = false);

#endif // QUARLCOIN_RPC_MEMPOOL_H
