// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.
//
// End-to-end test of the interfaces::Chain adapter (interfaces/chain.cpp): stand
// up a regtest ChainstateManager + mempool + ValidationSignals (immediate task
// runner), wrap it with MakeChain, and check
//   - the read surface: getHeight/getBlockHash/haveBlockOnDisk/findBlock/
//     havePruned/isInMempool on the genesis-only chain, and
//   - the notification bridge: after subscribing via handleNotifications, mining
//     block 1 delivers a blockConnected with the right height, hash and block data.

#include <interfaces/chain.h>
#include <key.h>
#include <interfaces/handler.h>

#include <validation.h>
#include <node/miner.h>
#include <validationinterface.h>
#include <node/chainstate.h>
#include <node/blockstorage.h>
#include <kernel/context.h>
#include <kernel/chain.h>
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
#include <primitives/transaction_identifier.h>
#include <script/script.h>
#include <pow.h>
#include <hash.h>
#include <sync.h>
#include <uint256.h>
#include <util/signalinterrupt.h>
#include <util/task_runner.h>
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

struct TestNotifications : public interfaces::Chain::Notifications {
    int connected{0};
    int last_height{-1};
    uint256 last_hash;
    bool had_data{false};
    void blockConnected(const kernel::ChainstateRole&, const interfaces::BlockInfo& block) override
    {
        ++connected;
        last_height = block.height;
        last_hash = block.hash;
        had_data = (block.data != nullptr && !block.data->vtx.empty());
    }
};
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

    const char* lad = std::getenv("LOCALAPPDATA");
    const std::string base = (lad && *lad) ? (std::string(lad) + "/quarlcoin_chain_iface_test")
                                           : std::string("quarlcoin_chain_iface_test");
    const std::filesystem::path dd(base);
    std::filesystem::remove_all(dd);
    std::filesystem::create_directories(dd / "blocks");
    const fs::path datadir = fs::PathFromString(base);
    const fs::path blocksdir = fs::PathFromString(base + "/blocks");

    kernel::Context context;
    kernel::Notifications notifications;
    util::SignalInterrupt interrupt;
    const std::unique_ptr<const CChainParams> params = CChainParams::RegTest();

    SetMockTime(params->GenesisBlock().nTime + 600);

    // A validation-signal bus with an immediate (synchronous) task runner, wired
    // into the chainstate so block-connected events fire through it.
    ValidationSignals validation_signals{std::make_unique<util::ImmediateTaskRunner>()};

    ChainstateManager::Options chainman_opts{
        .chainparams = *params,
        .datadir = datadir,
        .notifications = notifications,
        .signals = &validation_signals,
    };
    node::BlockManager::Options blockman_opts{
        .chainparams = *params,
        .use_xor = false,
        .blocks_dir = blocksdir,
        .notifications = notifications,
        .block_tree_db_params = DBParams{
            .path = blocksdir,
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

    {
        BlockValidationState state;
        Check("activate genesis chain", chainman.ActiveChainstate().ActivateBestChain(state, nullptr));
    }

    // Build the chain adapter under test.
    std::unique_ptr<interfaces::Chain> chain = interfaces::MakeChain(chainman, mempool, validation_signals);

    const uint256 genesis_hash = params->GetConsensus().hashGenesisBlock;

    // Read surface on the genesis-only chain.
    Check("getHeight == 0 (genesis only)", chain->getHeight() == std::optional<int>(0));
    Check("getBlockHash(0) == genesis", chain->getBlockHash(0) == genesis_hash);
    Check("haveBlockOnDisk(0)", chain->haveBlockOnDisk(0));
    Check("havePruned() == false", !chain->havePruned());
    Check("getPruneHeight() == nullopt", chain->getPruneHeight() == std::nullopt);
    {
        int h = -99;
        int64_t t = 0;
        bool ok = chain->findBlock(genesis_hash, interfaces::FoundBlock().height(h).time(t));
        Check("findBlock(genesis) found with height 0", ok && h == 0 && t == (int64_t)params->GenesisBlock().nTime);
        bool unknown = chain->findBlock(uint256::ONE, interfaces::FoundBlock());
        Check("findBlock(unknown) not found", !unknown);
    }

    // Subscribe to notifications (after genesis is already connected).
    auto notif = std::make_shared<TestNotifications>();
    std::unique_ptr<interfaces::Handler> handler = chain->handleNotifications(notif);

    // Block 1, mined the way a miner would.
    Check("mined and connected block 1",
          MineNextBlock(chainman, &mempool));

    uint256 block1_hash;
    Txid block1_coinbase;
    {
        LOCK(chainman.GetMutex());
        if (chainman.ActiveTip()) {
            block1_hash = chainman.ActiveTip()->GetBlockHash();
            CBlock b;
            if (chainman.m_blockman.ReadBlock(b, *chainman.ActiveTip())) block1_coinbase = b.vtx[0]->GetHash();
        }
    }

    chain->waitForNotifications();

    // The adapter delivered blockConnected for block 1 with the right payload.
    Check("blockConnected fired exactly once", notif->connected == 1);
    Check("blockConnected height == 1", notif->last_height == 1);
    Check("blockConnected hash == block1", notif->last_hash == block1_hash);
    Check("blockConnected carried block data", notif->had_data);

    // Read surface reflects the new tip.
    Check("getHeight == 1 after block 1", chain->getHeight() == std::optional<int>(1));
    Check("getBlockHash(1) == block1", chain->getBlockHash(1) == block1_hash);
    Check("isInMempool(coinbase) == false", !chain->isInMempool(block1_coinbase));

    handler->disconnect();
    chain->waitForNotifications();

    std::filesystem::remove_all(dd);
    std::printf("%s\n", g_fail == 0 ? "ALL PASS" : "FAILURES");
    return g_fail == 0 ? 0 : 1;
}
