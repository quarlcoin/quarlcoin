// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.
//
// Quarlcoin network parameters. Unlike Bitcoin Core's kernel/chainparams.cpp
// (which defines the Bitcoin network), this file defines Quarlcoin's networks
// and genesis blocks. The structure mirrors Core; the values are Quarlcoin's.
//
// The genesis blocks were mined with this chain's proof of work (grind nNonce
// until GetPoWHash of the 80-byte header is at or under the nBits target). The nNonce /
// hashGenesisBlock values below are the mining output and are reproduced +
// validated by test/test_chainparams.
//
// The identity values -- network magic, ports, address prefixes, bech32 HRPs
// and the extended-key versions -- are settled, and none of them is shared with
// another chain: see the note above base58Prefixes in CMainParams.

#include <kernel/chainparams.h>

#include <arith_uint256.h>
#include <consensus/amount.h>
#include <consensus/merkle.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <uint256.h>
#include <util/chaintype.h>

#include <limits>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.version = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block.
 *
 * The coinbase carries Satoshi's line, unchanged. It is a real Times front page
 * of 3 January 2009 and it says what this chain is for as exactly as it said
 * what Bitcoin was for: a state deciding, again, to make the holders of a
 * currency pay for the failures of the people who issue it.
 *
 * One property is given up by quoting it rather than quoting a paper of the day
 * this chain opened. Satoshi's line proved a date -- nobody could know that
 * front page in advance, so the block could not have been made earlier than it
 * claimed. A 2009 headline in a 2026 block proves nothing about 2026. What
 * fixes the date here instead is nTime, which the network checks against its own
 * clocks, and the difficulty the nonce was found under.
 *
 * The single output is OP_RETURN. There is no key in the genesis block, so
 * nobody can later produce a signature claiming to have written it, and no name
 * appears anywhere in it. The chain is anonymous from its first block.
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << OP_RETURN;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void CChainParams::ApplyDeploymentOptions(const DeploymentOptions& opts)
{
    for (const auto& [dep, height] : opts.activation_heights) {
        switch (dep) {
        case Consensus::BuriedDeployment::DEPLOYMENT_SEGWIT:
            consensus.SegwitHeight = int{height};
            break;
        case Consensus::BuriedDeployment::DEPLOYMENT_HEIGHTINCB:
            consensus.BIP34Height = int{height};
            break;
        case Consensus::BuriedDeployment::DEPLOYMENT_DERSIG:
            consensus.BIP66Height = int{height};
            break;
        case Consensus::BuriedDeployment::DEPLOYMENT_CLTV:
            consensus.BIP65Height = int{height};
            break;
        case Consensus::BuriedDeployment::DEPLOYMENT_CSV:
            consensus.CSVHeight = int{height};
            break;
        }
    }

    for (const auto& [deployment_pos, version_bits_params] : opts.version_bits_parameters) {
        consensus.vDeployments[deployment_pos].nStartTime = version_bits_params.start_time;
        consensus.vDeployments[deployment_pos].nTimeout = version_bits_params.timeout;
        consensus.vDeployments[deployment_pos].min_activation_height = version_bits_params.min_activation_height;
    }
}

/**
 * Main network on which people trade goods and services.
 */
class CMainParams : public CChainParams {
public:
    CMainParams(const MainNetOptions& opts) {
        m_chain_type = ChainType::MAIN;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        // 840,000 blocks at 150 seconds is the same four years Bitcoin halves on.
        consensus.nSubsidyHalvingInterval = 840000;
        // Quarlcoin enforces its rules from the first block it can, which is one
        // and not zero.
        //
        // Genesis is exempt, and not as a concession: it is accepted by a
        // hardcoded hash, so no rule applied to it can change which chain is
        // valid, and the contextual checks read a parent it does not have. Zero
        // here makes ContextualCheckBlock assert on genesis -- which is why
        // -reindex aborted on every release before this one. Core uses one for this
        // same reason on every chain whose rules start at the beginning.
        //
        // Nothing else moves. For the block at height one the test is 1 >= 0
        // against 1 >= 1, true either way, and it stays true for every block
        // above it. Genesis is the only block whose treatment changes.
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        // Segwit stays at zero: its check never reads the parent.
        consensus.SegwitHeight = 0;
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256{"00000000ffff0000000000000000000000000000000000000000000000000000"};
        consensus.nPowTargetSpacing = 150;                // two and a half minutes
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.enforce_BIP94 = false;
        consensus.fPowNoRetargeting = false;
        // ASERT, anchored at genesis: no activation height, no second algorithm
        // in the chain's past, and every target a function of this one block.
        // Twelve hours is 288 blocks here, the same inertia Bitcoin Cash gets
        // from two days at ten-minute blocks.
        consensus.nASERTHalfLife = 12 * 60 * 60;
        consensus.asert_anchor = {.nHeight = 0, .nBits = 0x1d00ffff, .nTime = 1785762002};
        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY] = Consensus::BIP9Deployment{};

        // The chain's own tag: a high byte that tells the three networks apart,
        // then "QRL". Bitcoin's f9beb4d9 belongs to Bitcoin and Litecoin's
        // fbc0b6db to Litecoin; this one belongs here, and says so.
        pchMessageStart[0] = 0xf1;
        pchMessageStart[1] = 0x51;  // Q
        pchMessageStart[2] = 0x52;  // R
        pchMessageStart[3] = 0x4c;  // L
        // Above 1024, so an ordinary user can bind it without privileges, and
        // below 32768, so it sits outside the range the kernel hands out for
        // outgoing connections (/proc/sys/net/ipv4/ip_local_port_range). A
        // listening port inside that range is one an outbound socket can take
        // first, and the failure then arrives at random.
        nDefaultPort = 9743;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(1785762002, 4012788493, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256{"2f5ec90129e2cb5bbc26a2e972898e7b22a635344f0cf524117021bbb46cb321"});
        assert(genesis.hashMerkleRoot == uint256{"8b62345f8f81e8215ebfc4875a69e2a02376061d78c30de06a48c63fcbee53c1"});

        // One DNS seed, and no addresses compiled in.
        //
        // A DNS seed decides which peers a node meets before it has met any, so
        // whoever answers this name chooses which chain a newcomer sees first.
        // That makes the name itself a consensus-adjacent asset: it MUST be
        // registered and under this project's control before any binary
        // carrying it is released. A name nobody has registered is worse than
        // no name at all -- it leaves that choice lying in the open for whoever
        // registers it next.
        //
        // It is only a first introduction. A node that reaches any peer fills
        // peers.dat from the peers themselves and never needs the name again,
        // and -addnode or -connect replaces it entirely for anyone who would
        // rather not ask.
        vSeeds.emplace_back("seed.quarlcoin.org");
        vFixedSeeds.clear();

        // Laid out as Core lays it out. Two addresses that read as a pair --
        // 58 gives 'Q', 60 gives 'R' -- which is what Bitcoin does with '1' and
        // '3' and Litecoin with 'L' and 'M'. The secret-key version is the
        // public-key version plus 128, as on every chain that took this from
        // Bitcoin. An extended key is two bytes naming the network and two
        // separating public from private: Bitcoin uses 0488 and 0435, Litecoin
        // 019D, Dogecoin 02FA, and 0E5C and 0E3F are free.
        //
        // What stood here was Bitcoin's own pair, 0488B21E and 0488ADE4, copied
        // unchanged. Any wallet or explorer reading a key of this chain would
        // have called it Bitcoin, which is the leak this set exists to close.
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 58);   // Q
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 60);   // R
        base58Prefixes[SECRET_KEY]     = std::vector<unsigned char>(1, 58 + 128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x0E, 0x5C, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x0E, 0x5C, 0xAD, 0xE4};
        bech32_hrp = "qrl";

        fDefaultConsistencyChecks = false;
        m_is_mockable_chain = false;

        chainTxData = ChainTxData{0, 0, 0};

        ApplyDeploymentOptions(opts.dep_opts);
    }
};

/**
 * Testnet: a public test network with its own genesis.
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams(const TestNetOptions& opts) {
        m_chain_type = ChainType::TESTNET;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        // 840,000 blocks at 150 seconds is the same four years Bitcoin halves on.
        consensus.nSubsidyHalvingInterval = 840000;
        // One and not zero, for the reason spelled out in CMainParams.
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 0;
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256{"00000000ffff0000000000000000000000000000000000000000000000000000"};
        consensus.nPowTargetSpacing = 150;
        // ASERT drops the difficulty on its own as soon as blocks run late,
        // so the twenty-minute escape hatch has nothing left to do.
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.enforce_BIP94 = false;
        consensus.fPowNoRetargeting = false;
        // ASERT, anchored at genesis: no activation height, no second algorithm
        // in the chain's past, and every target a function of this one block.
        // Twelve hours is 288 blocks here, the same inertia Bitcoin Cash gets
        // from two days at ten-minute blocks.
        consensus.nASERTHalfLife = 12 * 60 * 60;
        consensus.asert_anchor = {.nHeight = 0, .nBits = 0x1d00ffff, .nTime = 1785760200};
        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY] = Consensus::BIP9Deployment{};

        // Superseded and left standing on purpose: 200 was 8.7 minutes of a 20B
        // model at 2.62 s a step, and the first epoch trains 1.5B. The figure
        // that replaces it is 8.7 minutes of *that* step, which is estimated
        // from FLOPs and not measured -- so replacing one unmeasured number
        // with another buys nothing. Set it when a step has been timed; until
        // the fork is scheduled nothing reads it. See §1 and §10.
        pchMessageStart[0] = 0x0f;
        pchMessageStart[1] = 0x51;
        pchMessageStart[2] = 0x52;
        pchMessageStart[3] = 0x4c;
        nDefaultPort = 19743;  // see the note on mainnet
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(1785760200, 1429613558, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256{"23fa0a7b116b80b5ddb89d00d91ed8f51783f75bc4658eace7db1c37741ba75a"});
        assert(genesis.hashMerkleRoot == uint256{"8b62345f8f81e8215ebfc4875a69e2a02376061d78c30de06a48c63fcbee53c1"});

        // See the note on mainnet; the same condition applies to this name.
        vSeeds.emplace_back("testnet-seed.quarlcoin.org");
        vFixedSeeds.clear();

        // A test address must not look like a real one, and on the test chains
        // that is the whole of what these bytes are for -- they do not carry the
        // chain's letter. Bitcoin's testnet reads m/n, 2 and c; these are the
        // same shapes on bytes Bitcoin, Litecoin and Dogecoin do not use.
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 110);  // m
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 150);  // 2
        base58Prefixes[SECRET_KEY]     = std::vector<unsigned char>(1, 110 + 128);  // c
        base58Prefixes[EXT_PUBLIC_KEY] = {0x0E, 0x3F, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x0E, 0x3F, 0x83, 0x94};
        bech32_hrp = "tqrl";

        fDefaultConsistencyChecks = false;
        m_is_mockable_chain = false;

        chainTxData = ChainTxData{0, 0, 0};

        ApplyDeploymentOptions(opts.dep_opts);
    }
};

/**
 * Regression test: intended for private networks, which are not intended to be
 * publicly accessible and is used for testing. Difficulty never retargets.
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const RegTestOptions& opts) {
        m_chain_type = ChainType::REGTEST;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 150;
        // One and not zero, for the reason spelled out in CMainParams.
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 0;
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256{"7fffff0000000000000000000000000000000000000000000000000000000000"};
        consensus.nPowTargetSpacing = 150;
        // ASERT drops the difficulty on its own as soon as blocks run late,
        // so the twenty-minute escape hatch has nothing left to do.
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.enforce_BIP94 = opts.enforce_bip94;
        consensus.fPowNoRetargeting = true;
        // ASERT, anchored at genesis: no activation height, no second algorithm
        // in the chain's past, and every target a function of this one block.
        // Twelve hours is 288 blocks here, the same inertia Bitcoin Cash gets
        // from two days at ten-minute blocks.
        // Set anyway, so that a test which turns retargeting on gets the same
        // rule the other two networks have rather than an unset anchor.
        consensus.nASERTHalfLife = 12 * 60 * 60;
        consensus.asert_anchor = {.nHeight = 0, .nBits = 0x207fffff, .nTime = 1785758400};
        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY] = Consensus::BIP9Deployment{};

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0x51;
        pchMessageStart[2] = 0x52;
        pchMessageStart[3] = 0x4c;
        nDefaultPort = 19844;
        nPruneAfterHeight = opts.fastprune ? 100 : 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        genesis = CreateGenesisBlock(1785758400, 5, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256{"94bc8cee75d4e4ffe6f5abb2ba8d4ca6b3c2bf8dadf56ad8d994530e9d005866"});
        assert(genesis.hashMerkleRoot == uint256{"8b62345f8f81e8215ebfc4875a69e2a02376061d78c30de06a48c63fcbee53c1"});

        vSeeds.clear();
        vFixedSeeds.clear();

        // A test address must not look like a real one, and on the test chains
        // that is the whole of what these bytes are for -- they do not carry the
        // chain's letter. Bitcoin's testnet reads m/n, 2 and c; these are the
        // same shapes on bytes Bitcoin, Litecoin and Dogecoin do not use.
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 110);  // m
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 150);  // 2
        base58Prefixes[SECRET_KEY]     = std::vector<unsigned char>(1, 110 + 128);  // c
        base58Prefixes[EXT_PUBLIC_KEY] = {0x0E, 0x3F, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x0E, 0x3F, 0x83, 0x94};
        bech32_hrp = "qrlrt";

        fDefaultConsistencyChecks = true;
        m_is_mockable_chain = true;

        chainTxData = ChainTxData{0, 0, 0};

        ApplyDeploymentOptions(opts.dep_opts);
    }
};

std::vector<int> CChainParams::GetAvailableSnapshotHeights() const
{
    std::vector<int> heights;
    heights.reserve(m_assumeutxo_data.size());

    for (const auto& data : m_assumeutxo_data) {
        heights.push_back(data.height);
    }
    return heights;
}

std::optional<ChainType> GetNetworkForMagic(const MessageStartChars& message)
{
    const auto mainnet_msg = CChainParams::Main()->MessageStart();
    const auto testnet_msg = CChainParams::TestNet()->MessageStart();
    const auto regtest_msg = CChainParams::RegTest()->MessageStart();

    if (std::ranges::equal(message, mainnet_msg)) {
        return ChainType::MAIN;
    } else if (std::ranges::equal(message, testnet_msg)) {
        return ChainType::TESTNET;
    } else if (std::ranges::equal(message, regtest_msg)) {
        return ChainType::REGTEST;
    }
    return std::nullopt;
}

std::unique_ptr<const CChainParams> CChainParams::RegTest(const RegTestOptions& options)
{
    return std::make_unique<const CRegTestParams>(options);
}

std::unique_ptr<const CChainParams> CChainParams::Main(const MainNetOptions& options)
{
    return std::make_unique<const CMainParams>(options);
}

std::unique_ptr<const CChainParams> CChainParams::TestNet(const TestNetOptions& options)
{
    return std::make_unique<const CTestNetParams>(options);
}

// SigNet and TestNet4 are Bitcoin-specific networks (BIP325 signed blocks /
// the 2024 testnet reset). Quarlcoin does not define them yet; they will be
// added with their own mined genesis if/when needed.
std::unique_ptr<const CChainParams> CChainParams::SigNet(const SigNetOptions&)
{
    throw std::runtime_error("Quarlcoin: signet is not defined yet");
}

std::unique_ptr<const CChainParams> CChainParams::TestNet4(const TestNetOptions&)
{
    throw std::runtime_error("Quarlcoin: testnet4 is not defined yet");
}
