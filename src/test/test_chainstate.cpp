// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.
//
// End-to-end sanity check of the validation engine: stand up a minimal
// ChainstateManager on regtest (in-memory UTXO db, a temp block-file dir),
// load and activate the genesis block, then build a valid block 1 (coinbase
// with the BIP34 height, mined to satisfy the proof-of-work target) and drive it
// through ProcessNewBlock -> AcceptBlock -> ConnectBlock -> the
// UTXO set, confirming the active chain advances to height 1.

#include <validation.h>
#include <node/miner.h>
#include <key.h>
#include <node/chainstate.h>
#include <node/blockstorage.h>
#include <kernel/context.h>
#include <kernel/notifications_interface.h>
#include <kernel/caches.h>
#include <kernel/chainstatemanager_opts.h>
#include <kernel/blockmanager_opts.h>
#include <kernel/mempool_options.h>
#include <txmempool.h>
#include <consensus/validation.h>
#include <consensus/merkle.h>
#include <consensus/amount.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <pow.h>
#include <hash.h>
#include <sync.h>
#include <util/signalinterrupt.h>
#include <util/time.h>
#include <util/fs.h>
#include <dbwrapper.h>
#include <util/translation.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>

const TranslateFn G_TRANSLATION_FUN{nullptr};

namespace {
int g_fail = 0;
void Check(const char* name, bool ok)
{
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++g_fail;
}
} // namespace

namespace {
/** Mine the next block on the tip and connect it.
 *
 *  This used to call a shared helper that also produced the training a block
 *  was once required to carry. Training is gone, so a block is a block again:
 *  ask the assembler for a template, grind the nonce -- which on regtest takes
 *  a handful of tries -- and hand it to validation.
 */
bool MineNextBlock(ChainstateManager& chainman, CTxMemPool* mempool)
{
    const node::BlockCreateOptions opts{};
    std::unique_ptr<node::CBlockTemplate> tmpl{
        node::BlockAssembler{chainman.ActiveChainstate(), mempool, opts}.CreateNewBlock()};
    if (!tmpl) { std::printf("       no template\n"); return false; }

    auto block = std::make_shared<CBlock>(tmpl->block);
    // The assembler leaves the merkle root unset -- filling it in is the miner's
    // job, because a miner is expected to put its own extra nonce in the
    // coinbase first and that changes the root. Nothing is changed here, but the
    // root still has to be computed, or the block is rejected as
    // bad-txnmrklroot.
    block->hashMerkleRoot = BlockMerkleRoot(*block);

    while (!CheckProofOfWork(block->GetPoWHash(), block->nBits, chainman.GetConsensus())) {
        ++block->nNonce;
    }
    bool new_block{false};
    const bool accepted = chainman.ProcessNewBlock(block, /*force_processing=*/true,
                                                   /*min_pow_checked=*/true, &new_block);
    if (!accepted || !new_block) {
        const BlockValidationState st{WITH_LOCK(::cs_main, return TestBlockValidity(
            chainman.ActiveChainstate(), *block, /*check_pow=*/true, /*check_merkle_root=*/true))};
        std::printf("       accepted=%d new_block=%d reason=%s debug=%s\n",
                    accepted, new_block, st.GetRejectReason().c_str(), st.GetDebugMessage().c_str());
    }
    return accepted && new_block;
}
} // namespace

int main()
{
    // The curve, before anything asks it a question. Only the daemon used to
    // build this; a standalone test that signs or derives a key starts with a
    // null secp256k1 context and dies inside the first call.
    ECC_Context ecc_context;

    // Use a pure-ASCII absolute temp path: the repo lives under a Cyrillic
    // directory, and fs::path's u8 conversion would mangle a non-ASCII path.
    const char* lad = std::getenv("LOCALAPPDATA");
    const std::string base = (lad && *lad) ? (std::string(lad) + "/quarlcoin_chainstate_test")
                                           : std::string("quarlcoin_chainstate_test");
    const std::filesystem::path dd(base);
    std::filesystem::remove_all(dd);
    std::filesystem::create_directories(dd / "blocks");
    const fs::path datadir = fs::PathFromString(base);
    const fs::path blocksdir = fs::PathFromString(base + "/blocks");

    kernel::Context context;
    kernel::Notifications notifications;
    util::SignalInterrupt interrupt;
    const std::unique_ptr<const CChainParams> params = CChainParams::RegTest();

    // The genesis timestamp is in the future relative to the real clock, so pin
    // the node clock just past it; otherwise block 1 is rejected as time-too-new.
    SetMockTime(params->GenesisBlock().nTime + 600);

    ChainstateManager::Options chainman_opts{
        .chainparams = *params,
        .datadir = datadir,
        .notifications = notifications,
    };
    node::BlockManager::Options blockman_opts{
        .chainparams = *params,
        .use_xor = false,
        .blocks_dir = blocksdir,
        .notifications = notifications,
        .block_tree_db_params = DBParams{
            .path = blocksdir,        // unused: in-memory
            .cache_bytes = 1 << 20,
            .memory_only = true,
        },
    };
    ChainstateManager chainman{interrupt, chainman_opts, blockman_opts};

    kernel::MemPoolOptions mpopts;
    bilingual_str mperr;
    CTxMemPool mempool{mpopts, mperr};

    kernel::CacheSizes cache_sizes{1 << 22};
    node::ChainstateLoadOptions load_opts;
    load_opts.mempool = &mempool;
    load_opts.coins_db_in_memory = true;

    const auto [status, load_err] = node::LoadChainstate(chainman, cache_sizes, load_opts);
    Check("LoadChainstate success", status == node::ChainstateLoadStatus::SUCCESS);

    // Activate the genesis block (height 0).
    {
        BlockValidationState state;
        bool ok = chainman.ActiveChainstate().ActivateBestChain(state, nullptr);
        Check("activate genesis chain", ok);
    }
    {
        LOCK(chainman.GetMutex());
        Check("active height == 0 (genesis)", chainman.ActiveHeight() == 0);
        Check("tip is genesis", chainman.ActiveTip() && chainman.ActiveTip()->GetBlockHash() == params->GetConsensus().hashGenesisBlock);
    }

    // Block 1, mined the way a miner would.
    const bool mined = MineNextBlock(chainman, &mempool);

    Check("mined and connected block 1", mined);

    uint256 block1_hash;
    Txid block1_coinbase;
    if (mined) {
        LOCK(chainman.GetMutex());
        block1_hash = chainman.ActiveTip()->GetBlockHash();
        CBlock b;
        if (chainman.m_blockman.ReadBlock(b, *chainman.ActiveTip())) block1_coinbase = b.vtx[0]->GetHash();
    }

    {
        LOCK(chainman.GetMutex());
        Check("active height == 1 after block 1", chainman.ActiveHeight() == 1);
        Check("tip is block 1", chainman.ActiveTip() && chainman.ActiveTip()->GetBlockHash() == block1_hash);
        // The coinbase output is now a spendable UTXO.
        const Coin& coin = chainman.ActiveChainstate().CoinsTip().AccessCoin(COutPoint(block1_coinbase, 0));
        Check("coinbase UTXO present at height 1", !coin.IsSpent() && coin.nHeight == 1 && coin.IsCoinBase());
    }

    std::filesystem::remove_all(dd);
    std::printf("%s\n", g_fail == 0 ? "ALL PASS" : "FAILURES");
    return g_fail == 0 ? 0 : 1;
}
