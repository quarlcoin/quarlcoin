// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.
//
// Re-mines the genesis block of each network and prints the three values
// chainparams asserts on: the nonce, the merkle root and the block hash.
//
// It exists because those values are outputs, not choices. They follow from the
// headline in the coinbase, the timestamp, the difficulty and the hash functions
// in force, and any change to the last of those invalidates them. Run it, paste
// the output into kernel/chainparams.cpp, and the asserts hold again.

#include <arith_uint256.h>
#include <chainparamsbase.h>
#include <consensus/amount.h>
#include <consensus/merkle.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <uint256.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace {

CBlock BuildGenesis(const char* headline, uint32_t nTime, uint32_t nNonce, uint32_t nBits)
{
    CMutableTransaction txNew;
    txNew.version = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4)
                                       << std::vector<unsigned char>((const unsigned char*)headline,
                                                                     (const unsigned char*)headline + std::strlen(headline));
    txNew.vout[0].nValue = 50 * COIN;
    txNew.vout[0].scriptPubKey = CScript() << OP_RETURN;

    CBlock genesis;
    genesis.nTime = nTime;
    genesis.nBits = nBits;
    genesis.nNonce = nNonce;
    genesis.nVersion = 1;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

void Grind(const char* name, const char* headline, uint32_t nTime, uint32_t nBits);

void Grind(const char* name, const char* headline, uint32_t nTime, uint32_t nBits)
{
    CBlock header{BuildGenesis(headline, nTime, 0, nBits)};
    const uint256 merkle{header.hashMerkleRoot};

    arith_uint256 target;
    bool negative = false, overflow = false;
    target.SetCompact(nBits, &negative, &overflow);

    const unsigned threads = std::max(1u, std::thread::hardware_concurrency());
    std::atomic<bool> found{false};
    std::atomic<uint32_t> winner{0};
    std::atomic<uint64_t> tried{0};
    const auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> pool;
    for (unsigned t = 0; t < threads; ++t) {
        pool.emplace_back([&, t] {
            CBlock local{header};
            uint64_t local_tried = 0;
            for (uint64_t n = t; n <= 0xffffffffULL && !found.load(std::memory_order_relaxed); n += threads) {
                local.nNonce = static_cast<uint32_t>(n);
                if (UintToArith256(local.GetPoWHash()) <= target) {
                    winner.store(local.nNonce);
                    found.store(true);
                    break;
                }
                if ((++local_tried & 0xfffff) == 0) tried.fetch_add(0x100000, std::memory_order_relaxed);
            }
            tried.fetch_add(local_tried & 0xfffff, std::memory_order_relaxed);
        });
    }
    for (auto& th : pool) th.join();

    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    const uint64_t n = tried.load();

    if (!found.load()) {
        // The nonce is 32 bits and a target this low is not always reachable
        // within it. Bitcoin's answer is to move the timestamp, which changes
        // the header and so the whole search space; one second is enough.
        std::printf("%-8s no nonce at nTime=%u (%.1f s, %.1f MH/s), trying %u\n",
                    name, nTime, secs, n / secs / 1e6, nTime + 1);
        Grind(name, headline, nTime + 1, nBits);
        return;
    }

    CBlock solved{BuildGenesis(headline, nTime, winner.load(), nBits)};
    std::printf("%-8s nTime=%u nNonce=%u\n", name, nTime, winner.load());
    std::printf("         hashGenesisBlock = %s\n", solved.GetHash().ToString().c_str());
    std::printf("         hashMerkleRoot   = %s\n", merkle.ToString().c_str());
    std::printf("         %.1f s, %.1f MH/s\n\n", secs, n / secs / 1e6);
}

} // namespace

int main(int argc, char* argv[])
{
    // No default. A default is a second copy of the line, and a second copy is
    // a tool that computes the constants for a chain other than the one in
    // kernel/chainparams.cpp -- silently, and with output that looks right.
    // That happened once; it cost a re-mine and nearly cost a wrong genesis.
    if (argc < 2 || argv[1][0] == '\0') {
        std::fprintf(stderr,
            "usage: gen_genesis \"<the pszTimestamp from kernel/chainparams.cpp>\" [regtest|testnet|mainnet]\n");
        return 1;
    }
    const char* headline = argv[1];
    std::printf("headline: %s\n\n", headline);

    const std::string only = (argc > 2) ? argv[2] : "";
    if (only.empty() || only == "regtest") Grind("regtest", headline, 1785758400, 0x207fffff);
    if (only.empty() || only == "testnet") Grind("testnet", headline, 1785760200, 0x1d00ffff);
    if (only.empty() || only == "mainnet") Grind("mainnet", headline, 1785762000, 0x1d00ffff);
    return 0;
}
