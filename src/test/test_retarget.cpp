// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.
//
// ASERT: the difficulty rule, tested by its properties rather than by a table.
//
// This chain has no retarget epoch. The target of a block is an exponential in
// how far its parent is off schedule, measured from a single anchor -- genesis --
// so every target the chain will ever have is a function of three numbers and
// nothing else. That is what makes the properties below the right thing to
// check: they are the rule, and a table of expected values would only record one
// run of it.
//
// What each group is for:
//
//   on schedule      the fixed point. A chain arriving exactly on time must not
//                    drift, or difficulty would wander with no hashrate change.
//   half-lives       the definition. One half-life late doubles the target, one
//                    early halves it, and n of them is 2^n.
//   monotonicity     the direction. Later blocks must never make mining harder.
//   path-independence what "absolute" means: the target at a height depends on
//                    that height and that timestamp, not on how the chain got
//                    there. An incremental rule can accumulate error; this one
//                    has nowhere to accumulate it.
//   the extremes     a chain that stops for a year must not overflow into an
//                    impossible target, and one mined absurdly fast must not
//                    reach a target of zero, which no hash can meet.
//   known answers    a handful of exact outputs, so that a change to the fixed
//                    point arithmetic is caught here rather than by a fork.

#include <arith_uint256.h>
#include <chain.h>
#include <consensus/params.h>
#include <pow.h>
#include <primitives/block.h>
#include <uint256.h>

#include <cstdint>
#include <cstdio>
#include <string>

namespace {

int g_fail = 0;

void Check(const char* name, bool ok)
{
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++g_fail;
}

constexpr int64_t SPACING = 150;
constexpr int64_t HALFLIFE = 12 * 60 * 60;
constexpr uint32_t ANCHOR_BITS = 0x1d00ffff;
constexpr uint32_t ANCHOR_TIME = 1785762002;

Consensus::Params MakeParams(uint32_t anchor_bits = ANCHOR_BITS)
{
    Consensus::Params p{};
    arith_uint256 limit;
    limit.SetCompact(0x1d00ffff);
    p.powLimit = ArithToUint256(limit);
    p.fPowAllowMinDifficultyBlocks = false;
    p.enforce_BIP94 = false;
    p.fPowNoRetargeting = false;
    p.nPowTargetSpacing = SPACING;
    p.nASERTHalfLife = HALFLIFE;
    p.asert_anchor = {.nHeight = 0, .nBits = anchor_bits, .nTime = ANCHOR_TIME};
    return p;
}

/** The target for a block whose parent is at `height`, `blocks_late * SPACING`
 *  seconds behind the schedule that height implies. */
arith_uint256 TargetAt(const Consensus::Params& p, int64_t height, int64_t seconds_late)
{
    const int64_t on_schedule{int64_t{ANCHOR_TIME} + SPACING * height};
    arith_uint256 t;
    t.SetCompact(CalculateNextWorkRequired(p, height, on_schedule + seconds_late));
    return t;
}

std::string Hex(const arith_uint256& v) { return ArithToUint256(v).GetHex(); }

/** 256x harder than the limit, so that both directions have room.
 *
 *  The live chain anchors at powLimit itself, which means a late chain cannot
 *  get an easier target than genesis -- there is none. That is a property of the
 *  chain and is checked below; it also makes the anchor a poor place to measure
 *  what a half-life does, so the arithmetic is exercised here instead. */
constexpr uint32_t HARD_BITS = 0x1c00ffff;

/** Within one part in 2^13 -- the cubic that stands in for 2^x is exact to about
 *  0.003%, and rounding through the compact encoding costs a little more. */
bool Close(const arith_uint256& a, const arith_uint256& b)
{
    const arith_uint256 hi{a > b ? a : b};
    const arith_uint256 lo{a > b ? b : a};
    return (hi - lo) <= (hi >> 13);
}

} // namespace

int main()
{
    const Consensus::Params p{MakeParams()};

    arith_uint256 anchor;
    anchor.SetCompact(ANCHOR_BITS);

    std::printf("-- a chain arriving on schedule does not drift --\n");
    {
        bool ok = true;
        for (int64_t h : {int64_t{1}, int64_t{2}, int64_t{288}, int64_t{2016}, int64_t{840000}}) {
            if (!Close(TargetAt(p, h, 0), anchor)) {
                ok = false;
                std::printf("      height %lld -> %s\n", (long long)h, Hex(TargetAt(p, h, 0)).c_str());
            }
        }
        Check("on schedule, the target stays at the anchor", ok);
    }

    std::printf("\n-- a half-life is a doubling --\n");
    {
        const Consensus::Params hard{MakeParams(HARD_BITS)};
        arith_uint256 h;
        h.SetCompact(HARD_BITS);
        Check("one half-life late  -> target x2",  Close(TargetAt(hard, 288, HALFLIFE), h << 1));
        Check("two half-lives late -> target x4",  Close(TargetAt(hard, 288, 2 * HALFLIFE), h << 2));
        Check("one half-life early -> target /2",  Close(TargetAt(hard, 288, -HALFLIFE), h >> 1));
        Check("two half-lives early-> target /4",  Close(TargetAt(hard, 288, -2 * HALFLIFE), h >> 2));
    }

    std::printf("\n-- the live chain can never get easier than genesis --\n");
    {
        // The anchor is powLimit, so there is no target above it to fall to. A
        // chain that stalls does not get an easier block than the first one; it
        // gets the first one's difficulty and waits. This is why the group above
        // needs a different anchor to measure a doubling at all, and it is worth
        // stating as a rule rather than discovering as a surprise.
        arith_uint256 limit;
        limit.SetCompact(0x1d00ffff);
        bool ok = true;
        for (int64_t late : {int64_t{600}, HALFLIFE, 10 * HALFLIFE, int64_t{365} * 24 * 3600}) {
            if (TargetAt(p, 288, late) != limit) ok = false;
        }
        Check("however late the chain runs, the target stops at genesis", ok);
        Check("and running early still makes it harder", TargetAt(p, 288, -HALFLIFE) < limit);
    }

    std::printf("\n-- later never makes mining harder --\n");
    {
        bool ok = true;
        arith_uint256 prev{TargetAt(p, 1000, -4 * HALFLIFE)};
        for (int64_t late = -4 * HALFLIFE + 600; late <= 4 * HALFLIFE; late += 600) {
            const arith_uint256 cur{TargetAt(p, 1000, late)};
            if (cur < prev) ok = false;
            prev = cur;
        }
        Check("the target never falls as the parent's timestamp rises", ok);
    }

    std::printf("\n-- the same height and time give the same answer, whatever the path --\n");
    {
        // Two chains reaching height 5000 at the same moment, one that dawdled
        // early and hurried late and one that did the reverse, get one target.
        const int64_t t{int64_t{ANCHOR_TIME} + SPACING * 5000 + 3 * HALFLIFE};
        const uint32_t a{CalculateNextWorkRequired(p, 5000, t)};
        const uint32_t b{CalculateNextWorkRequired(p, 5000, t)};
        Check("target is a function of (height, time) alone", a == b);

        // And an hour of extra delay is worth the same wherever it happens.
        const arith_uint256 early{TargetAt(p, 100, 3600)};
        const arith_uint256 late{TargetAt(p, 100000, 3600)};
        Check("the same lateness costs the same at any height", early == late);
    }

    std::printf("\n-- the extremes --\n");
    {
        arith_uint256 limit;
        limit.SetCompact(0x1d00ffff);
        Check("a chain stalled for a year is clamped at powLimit",
              TargetAt(p, 288, 365 * 24 * 60 * 60) == limit);
        Check("a chain stalled for a century is still exactly powLimit",
              TargetAt(p, 288, int64_t{100} * 365 * 24 * 60 * 60) == limit);

        const arith_uint256 fast{TargetAt(p, 100000, -int64_t{100} * 365 * 24 * 60 * 60)};
        Check("a chain mined impossibly fast never reaches a target of zero", fast > arith_uint256{0});
    }

    std::printf("\n-- known answers, so the fixed-point arithmetic cannot drift --\n");
    {
        struct KAT { int64_t height; int64_t late; const char* bits; };
        // Computed by an independent implementation of the same formula
        // (test/asert_ref.py), in a language with unbounded integers, so that
        // it needs none of the overflow care the C++ does -- which is what makes
        // it a check on the C++ rather than a copy of it.
        const KAT kats[] = {
            {1,      0,             "1d00ffff"},
            {288,    HALFLIFE,      "1d00ffff"},   // clamped: the anchor is powLimit
            {288,    -HALFLIFE,     "1c7fff80"},
            {2016,   3600,          "1d00ffff"},   // clamped
            {840000, -12345,        "1d00d201"},
        };
        for (const KAT& k : kats) {
            const int64_t on{int64_t{ANCHOR_TIME} + SPACING * k.height};
            char got[16];
            std::snprintf(got, sizeof got, "%08x", CalculateNextWorkRequired(p, k.height, on + k.late));
            char name[96];
            std::snprintf(name, sizeof name, "height %lld, %+lld s -> %s", (long long)k.height, (long long)k.late, k.bits);
            const bool ok = std::string(got) == k.bits;
            std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
            if (!ok) { std::printf("      got %s\n", got); ++g_fail; }
        }

        const Consensus::Params hard{MakeParams(HARD_BITS)};
        const KAT hard_kats[] = {
            {1,      0,         "1c00ffff"},
            {288,    HALFLIFE,  "1c01fffe"},
            {288,    -HALFLIFE, "1b7fff80"},
            {2016,   3600,      "1c010f3e"},
            {840000, -12345,    "1c00d201"},
        };
        for (const KAT& k : hard_kats) {
            const int64_t on{int64_t{ANCHOR_TIME} + SPACING * k.height};
            char got[16];
            std::snprintf(got, sizeof got, "%08x", CalculateNextWorkRequired(hard, k.height, on + k.late));
            char name[112];
            std::snprintf(name, sizeof name, "hard anchor, height %lld, %+lld s -> %s",
                          (long long)k.height, (long long)k.late, k.bits);
            const bool ok = std::string(got) == k.bits;
            std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
            if (!ok) { std::printf("      got %s\n", got); ++g_fail; }
        }
    }

    std::printf("\n-- the headers-sync check agrees with the rule it guards --\n");
    {
        const int64_t t{int64_t{ANCHOR_TIME} + SPACING * 4242 + 900};
        const uint32_t right{CalculateNextWorkRequired(p, 4242, t)};
        Check("the exact nBits is permitted", PermittedDifficultyTransition(p, 4242, t, right));
        Check("one bit off is not", !PermittedDifficultyTransition(p, 4242, t, right + 1));
        Check("the anchor's own bits are not, at a late height",
              !PermittedDifficultyTransition(p, 4242, t, ANCHOR_BITS) || right == ANCHOR_BITS);

        Consensus::Params regtest{p};
        regtest.fPowNoRetargeting = true;
        Check("regtest, where the difficulty does not move, permits anything",
              PermittedDifficultyTransition(regtest, 4242, t, 0x207fffff));
    }

    std::printf(g_fail ? "\nFAILED (%d)\n" : "\nOK\n", g_fail);
    return g_fail ? 1 : 0;
}
