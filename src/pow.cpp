// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <util/check.h>

/** The ASERT target for a block whose parent is at `prev_height` with timestamp
 *  `prev_time`.
 *
 *  target = anchor_target * 2^((elapsed - spacing * blocks) / halflife)
 *
 *  where `elapsed` is the time from the anchor to the parent and `blocks` is the
 *  number of blocks over that stretch. The bracket is how far the chain is off
 *  schedule, in seconds: positive when blocks came slower than the spacing, and
 *  the target rises -- which is difficulty falling -- by one doubling per
 *  half-life.
 *
 *  Absolute, not incremental. Nothing here reads the parent's nBits, so the
 *  difficulty of a block is a function of the anchor and of that block's parent
 *  alone. A drifting error cannot accumulate because there is no accumulator,
 *  and a node can compute the target at any height without walking the chain to
 *  it.
 *
 *  The exponential is evaluated in fixed point with sixteen fractional bits:
 *  the integer part becomes a shift, and the fractional part goes through a
 *  cubic that approximates 2^x on [0,1) to within about 0.003%. The whole of it
 *  is integer arithmetic, because two nodes that computed this in floating point
 *  would eventually disagree in the last bit and split the chain. The
 *  coefficients are Bitcoin Cash's, from an implementation that has set the
 *  difficulty of a live chain since 2020.
 */
static arith_uint256 ASERTTarget(const Consensus::Params& params, int64_t prev_height, int64_t prev_time)
{
    const arith_uint256 pow_limit{UintToArith256(params.powLimit)};

    arith_uint256 anchor_target;
    anchor_target.SetCompact(params.asert_anchor.nBits);

    const int64_t blocks{prev_height - params.asert_anchor.nHeight};
    const int64_t elapsed{prev_time - int64_t{params.asert_anchor.nTime}};

    // How far off schedule, as a 16-bit fixed-point number of half-lives.
    int64_t exponent{((elapsed - params.nPowTargetSpacing * blocks) * 65536) / params.nASERTHalfLife};

    // Split into a whole number of doublings and a remainder in [0, 1).
    const int64_t shifts{exponent >> 16};
    exponent -= shifts * 65536;
    Assume(exponent >= 0 && exponent < 65536);

    // 2^exponent - 1, to sixteen fractional bits, by a cubic in the remainder.
    const uint64_t frac{static_cast<uint64_t>(exponent)};
    const uint64_t factor{65536 + ((195766423245049ULL * frac +
                                    971821376ULL * frac * frac +
                                    5127ULL * frac * frac * frac +
                                    (1ULL << 47)) >> 48)};

    arith_uint256 next{anchor_target * factor};

    if (shifts <= 0) {
        // A chain far ahead of schedule can ask for more doublings than there
        // are bits, which would shift the target to zero and make the next
        // block unmineable. One is the hardest target there is.
        if (-shifts >= 256) return arith_uint256{1};
        next >>= -shifts;
    } else {
        if (shifts >= 256) return pow_limit;
        const arith_uint256 shifted{next << shifts};
        // Shifting out the top bits would silently produce a small target, i.e.
        // an enormous difficulty, from a chain that is merely very late. With
        // wider integers the answer would be past the limit anyway, so it is
        // the limit.
        if ((shifted >> shifts) != next) return pow_limit;
        next = shifted;
    }

    next >>= 16;

    if (next == 0) return arith_uint256{1};
    if (next > pow_limit) return pow_limit;
    return next;
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader* /*pblock*/, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);

    // Regtest, where the difficulty never moves and blocks are made to order.
    if (params.fPowNoRetargeting) return pindexLast->nBits;

    return ASERTTarget(params, pindexLast->nHeight, pindexLast->GetBlockTime()).GetCompact();
}

unsigned int CalculateNextWorkRequired(const Consensus::Params& params, int64_t prev_height, int64_t prev_time)
{
    return ASERTTarget(params, prev_height, prev_time).GetCompact();
}

bool PermittedDifficultyTransition(const Consensus::Params& params, int64_t prev_height, int64_t prev_time, uint32_t new_nbits)
{
    if (params.fPowNoRetargeting) return true;

    // Exact, where the epoch rule could only be approximate.
    //
    // Under the old rule this function bounded how far difficulty could move
    // across a retarget, because a header on its own carried nothing to check
    // the new value against. ASERT makes the target a function of the parent's
    // height and timestamp, both of which a header sync already has, so the
    // check is not a bound on the change -- it is the answer.
    //
    // What it defends is unchanged: a peer feeding us a chain of headers cannot
    // claim more work than it did. Claiming an early timestamp to inflate the
    // difficulty only obliges it to have found hashes under that harder target.
    return new_nbits == ASERTTarget(params, prev_height, prev_time).GetCompact();
}

// Bypasses the actual proof of work check during fuzz testing with a simplified validation checking whether
// the most significant bit of the last byte of the hash is set.
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    if (EnableFuzzDeterminism()) return (hash.data()[31] & 0x80) == 0;
    return CheckProofOfWorkImpl(hash, nBits, params);
}

std::optional<arith_uint256> DeriveTarget(unsigned int nBits, const uint256 pow_limit)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(pow_limit))
        return {};

    return bnTarget;
}

bool CheckProofOfWorkImpl(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    auto bnTarget{DeriveTarget(nBits, params.powLimit)};
    if (!bnTarget) return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
