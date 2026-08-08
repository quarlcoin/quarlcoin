// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_CONSENSUS_PARAMS_H
#define QUARLCOIN_CONSENSUS_PARAMS_H

#include <script/verify_flags.h>
#include <uint256.h>

#include <array>
#include <chrono>
#include <limits>
#include <map>
#include <vector>

namespace Consensus {

/**
 * A buried deployment is one where the height of the activation has been hardcoded into
 * the client implementation long after the consensus change has activated. See BIP 90.
 * Consensus changes for which the new rules are enforced from genesis are not listed here.
 */
enum BuriedDeployment : int16_t {
    // buried deployments get negative values to avoid overlap with DeploymentPos
    DEPLOYMENT_HEIGHTINCB = std::numeric_limits<int16_t>::min(),
    DEPLOYMENT_CLTV,
    DEPLOYMENT_DERSIG,
    DEPLOYMENT_CSV,
    // SCRIPT_VERIFY_WITNESS is enforced from genesis, but the check for downloading
    // missing witness data is not. BIP 147 also relies on hardcoded activation height.
    DEPLOYMENT_SEGWIT,
};
constexpr bool ValidDeployment(BuriedDeployment dep) { return dep <= DEPLOYMENT_SEGWIT; }

enum DeploymentPos : uint16_t {
    DEPLOYMENT_TESTDUMMY,
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in deploymentinfo.cpp
    // Removing an entry may require bumping MinBIP9WarningHeight.
    MAX_VERSION_BITS_DEPLOYMENTS
};
constexpr bool ValidDeployment(DeploymentPos dep) { return dep < MAX_VERSION_BITS_DEPLOYMENTS; }

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit{28};
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime{NEVER_ACTIVE};
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout{NEVER_ACTIVE};
    /** If lock in occurs, delay activation until at least this block
     *  height. Activation only occurs on a period boundary.
     */
    int min_activation_height{0};
    /** Blocks in one BIP 9 signalling window. NOT a difficulty period.
     *
     *  There is no difficulty period on this chain. ASERT sets the target from
     *  the anchor on every block, so nothing anywhere counts blocks to decide
     *  when to retarget, and this number is read by exactly one thing: how many
     *  blocks a deployment's votes are counted over.
     *
     *  It is spelled out because the field it replaces was called `period` and
     *  on Bitcoin genuinely was the retarget period -- BIP 9 was written for a
     *  chain whose difficulty moved on 2016-block boundaries, so counting votes
     *  over the same stretch cost nothing. Reading this number as a retarget is
     *  therefore the natural mistake, and the name now says it is not one.
     *
     *  Still 2016: a voting window wants to be long enough that a short run of
     *  luck cannot carry a vote and short enough that a decision arrives in
     *  weeks. At 150-second blocks that is three and a half days, where
     *  Bitcoin's is two weeks. */
    uint32_t signalling_period{2016};
    /**
     * How many of those blocks must signal for the deployment to lock in.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t threshold{1916};

    /** Constant for nTimeout very far in the future. */
    static constexpr int64_t NO_TIMEOUT = std::numeric_limits<int64_t>::max();

    /** Special value for nStartTime indicating that the deployment is always active.
     *  This is useful for testing, as it means tests don't need to deal with the activation
     *  process (which takes at least 3 BIP9 intervals). Only tests that specifically test the
     *  behaviour during activation cannot use this. */
    static constexpr int64_t ALWAYS_ACTIVE = -1;

    /** Special value for nStartTime indicating that the deployment is never active.
     *  This is useful for integrating the code changes for a new feature
     *  prior to deploying it on some or all networks. */
    static constexpr int64_t NEVER_ACTIVE = -2;
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    int nSubsidyHalvingInterval;
    /**
     * Hashes of blocks that
     * - are known to be consensus valid, and
     * - buried in the chain, and
     * - fail if the default script verify flags are applied.
     */
    std::map<uint256, script_verify_flags> script_flag_exceptions;
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;
    uint256 BIP34Hash;
    /** Block height at which BIP65 becomes active */
    int BIP65Height;
    /** Block height at which BIP66 becomes active */
    int BIP66Height;
    /** Block height at which CSV (BIP68, BIP112 and BIP113) becomes active */
    int CSVHeight;
    /** Block height at which Segwit (BIP141, BIP143 and BIP147) becomes active.
     * Note that segwit v0 script rules are enforced on all blocks except the
     * BIP 16 exception blocks. */
    int SegwitHeight;
    /** Don't warn about unknown BIP 9 activations below this height.
     * This prevents us from warning about the CSV, segwit and taproot activations. */
    int MinBIP9WarningHeight;
    std::array<BIP9Deployment,MAX_VERSION_BITS_DEPLOYMENTS> vDeployments;
    /** Proof of work parameters */
    uint256 powLimit;
    bool fPowAllowMinDifficultyBlocks;
    /**
      * Enforce BIP94 timewarp attack mitigation. On testnet4 this also enforces
      * the block storm mitigation.
      */
    bool enforce_BIP94;
    bool fPowNoRetargeting;
    int64_t nPowTargetSpacing;
    std::chrono::seconds PowTargetSpacing() const
    {
        return std::chrono::seconds{nPowTargetSpacing};
    }

    /** How long the difficulty takes to halve, in seconds, when blocks run slow.
     *
     *  ASERT sets the target as an exponential in how far the chain is behind
     *  or ahead of schedule, and this is the time constant of that exponential:
     *  a chain a half-life behind its expected time halves its difficulty, and
     *  one a half-life ahead doubles it.
     *
     *  Twelve hours, which is 288 blocks at 150 seconds. The number is a choice
     *  between two failures. Too short and every block moves the difficulty a
     *  long way, so a run of lucky or unlucky blocks swings it about and a miner
     *  can game the timestamps; too long and hashrate that leaves takes days to
     *  be noticed, which is how a small chain freezes. Bitcoin Cash uses two
     *  days at ten-minute blocks -- 288 blocks -- and twelve hours here is the
     *  same number of blocks at this chain's spacing. */
    int64_t nASERTHalfLife;

    /** The block ASERT measures everything from.
     *
     *  Genesis, so there is no activation height, no rule that changes partway
     *  and no second difficulty algorithm anywhere in the chain's history. Every
     *  target the chain will ever have is a function of this one block, the
     *  height and timestamp of the block being extended, and nothing else -- so
     *  a node can compute the difficulty of any block without walking to it.
     *
     *  nTime is the anchor's own timestamp and not its parent's. Bitcoin Cash
     *  anchors mid-chain and so uses the parent's, carrying a `+1` on the height
     *  difference to match; anchored at genesis there is no parent, and the
     *  formula below drops both. That is a real difference in the equation and
     *  it is written here rather than left to be inferred. */
    struct ASERTAnchor {
        int nHeight;
        uint32_t nBits;
        uint32_t nTime;
    };
    ASERTAnchor asert_anchor;
    /** The best chain should have at least this much work */
    uint256 nMinimumChainWork;
    /** By default assume that the signatures in ancestors of this block are valid */
    uint256 defaultAssumeValid;

    /**
     * If true, witness commitments contain a payload equal to a Bitcoin Script solution
     * to the signet challenge. See BIP325.
     */
    bool signet_blocks{false};
    std::vector<uint8_t> signet_challenge;

    int DeploymentHeight(BuriedDeployment dep) const
    {
        switch (dep) {
        case DEPLOYMENT_HEIGHTINCB:
            return BIP34Height;
        case DEPLOYMENT_CLTV:
            return BIP65Height;
        case DEPLOYMENT_DERSIG:
            return BIP66Height;
        case DEPLOYMENT_CSV:
            return CSVHeight;
        case DEPLOYMENT_SEGWIT:
            return SegwitHeight;
        } // no default case, so the compiler can warn about missing cases
        return std::numeric_limits<int>::max();
    }
};

} // namespace Consensus

#endif // QUARLCOIN_CONSENSUS_PARAMS_H
