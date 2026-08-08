// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#include <primitives/block.h>

#include <hash.h>
#include <tinyformat.h>

#include <memory>
#include <sstream>

uint256 CBlockHeader::GetHash() const
{
    return (HashWriter{} << *this).GetHash();
}

uint256 CBlockHeader::GetPoWHash() const
{
    // One more SHA-256d over the block hash. The block hash is itself SHA-256d
    // (like every txid and the Merkle root); only this proof-of-work value hashes
    // the hash again, so a miner grinding it runs four compressions per nonce
    // where Bitcoin runs two.
    return Hash(GetHash());
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}
