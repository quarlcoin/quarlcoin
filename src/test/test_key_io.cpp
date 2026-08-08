// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.
//
// Tests for address encoding (key_io.cpp): the witness-program framing matches
// the canonical BIP173 segwit address (known-answer), Encode/Decode round-trips
// for P2WPKH, P2WSH, anchor and legacy P2PKH on a network's prefixes, and an
// address from another network is rejected.
//
// key_io reads the selected chain rather than taking one, so these switch the
// global selection instead of holding two CChainParams side by side.

#include <key_io.h>

#include <addresstype.h>
#include <bech32.h>
#include <chainparams.h>
#include <kernel/chainparams.h>
#include <key.h>
#include <pubkey.h>
#include <script/script.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <cstdio>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace {
int g_fail = 0;
void Check(const char* name, bool ok)
{
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++g_fail;
}
} // namespace

int main()
{
    // The curve, before anything asks it a question. Only the daemon used to
    // build this; a standalone test that signs or derives a key starts with a
    // null secp256k1 context and dies inside the first call.
    ECC_Context ecc_context;

    // Known-answer: the BIP173 P2WPKH example bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4
    // has program 751e76e8199196d454941c45d1b3a323f1433bd6. Reproduce key_io's v0
    // framing (version byte 0 + 8->5 bit conversion + bech32) with hrp "bc".
    {
        const std::vector<unsigned char> prog = {
            0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4, 0x54, 0x94,
            0x1c, 0x45, 0xd1, 0xb3, 0xa3, 0x23, 0xf1, 0x43, 0x3b, 0xd6};
        std::vector<unsigned char> data = {0};
        ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, prog.begin(), prog.end());
        const std::string addr = bech32::Encode(bech32::Encoding::BECH32, "bc", data);
        Check("v0 framing matches canonical BIP173 P2WPKH address",
              addr == "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4");
    }

    SelectParams(ChainType::REGTEST);

    const CKey key = GenerateRandomKey();
    const CPubKey pub = key.GetPubKey();

    // P2WPKH round-trip on regtest (hrp "qrlrt").
    {
        const CTxDestination dest = WitnessV0KeyHash(pub);
        const std::string addr = EncodeDestination(dest);
        Check("P2WPKH address is non-empty and uses the regtest HRP", addr.rfind("qrlrt1", 0) == 0);
        const CTxDestination back = DecodeDestination(addr);
        Check("P2WPKH decodes back to WitnessV0KeyHash", std::holds_alternative<WitnessV0KeyHash>(back));
        Check("P2WPKH re-encodes to the same address", EncodeDestination(back) == addr);
        Check("P2WPKH address is valid", IsValidDestinationString(addr));
    }

    // P2WSH round-trip.
    {
        const CScript witnessScript = CScript() << OP_TRUE;
        const CTxDestination dest = WitnessV0ScriptHash(witnessScript);
        const std::string addr = EncodeDestination(dest);
        Check("P2WSH address uses regtest HRP", addr.rfind("qrlrt1", 0) == 0 && addr.size() > 40);
        const CTxDestination back = DecodeDestination(addr);
        Check("P2WSH decodes back to WitnessV0ScriptHash", std::holds_alternative<WitnessV0ScriptHash>(back));
        Check("P2WSH re-encodes to the same address", EncodeDestination(back) == addr);
    }

    // Pay-to-anchor (witness v1, bech32m).
    {
        const std::string addr = EncodeDestination(PayToAnchor());
        Check("anchor address is non-empty (bech32m)", !addr.empty() && addr.rfind("qrlrt1", 0) == 0);
        const CTxDestination back = DecodeDestination(addr);
        Check("anchor decodes back to PayToAnchor", std::holds_alternative<PayToAnchor>(back));
        Check("anchor is a valid destination string", IsValidDestinationString(addr));
    }

    // Legacy base58. A lattice public key was too large for a bare P2PKH spend,
    // so these encoded to nothing; a 33-byte key fits the script limits that
    // were always there, and the type is addressable again.
    {
        const std::string addr = EncodeDestination(PKHash(pub));
        Check("P2PKH has a base58 address", !addr.empty());
        const CTxDestination back = DecodeDestination(addr);
        Check("P2PKH decodes back to PKHash", std::holds_alternative<PKHash>(back));
        Check("P2PKH re-encodes to the same address", EncodeDestination(back) == addr);
    }

    // A mainnet address must not decode on regtest.
    std::string main_addr;
    {
        SelectParams(ChainType::MAIN);
        main_addr = EncodeDestination(WitnessV0KeyHash(pub));
        Check("mainnet P2WPKH uses the main HRP", main_addr.rfind("qrl1", 0) == 0);
        SelectParams(ChainType::REGTEST);
        Check("mainnet address is invalid on regtest", !IsValidDestinationString(main_addr));
        Check("decoding cross-network yields CNoDestination",
              std::holds_alternative<CNoDestination>(DecodeDestination(main_addr)));
    }

    Check("garbage string is invalid", !IsValidDestinationString("not an address"));

    // Error reporting via the error_str / error_locations overload.
    {
        const std::string addr = EncodeDestination(WitnessV0KeyHash(pub));
        std::string err = "x";
        DecodeDestination(addr, err);
        Check("valid address leaves error_str empty", err.empty());

        std::string err2;
        DecodeDestination(main_addr, err2);
        Check("wrong-HRP decode reports a prefix error", !err2.empty());

        // Corrupt the last data character -> checksum fails -> bech32 error location.
        std::string bad = addr;
        bad.back() = (bad.back() == 'q') ? 'p' : 'q';
        std::string err3;
        std::vector<int> locs;
        const CTxDestination d = DecodeDestination(bad, err3, &locs);
        Check("corrupted address reports an error and is no destination",
              !err3.empty() && std::holds_alternative<CNoDestination>(d));
    }

    std::printf("%s\n", g_fail == 0 ? "ALL PASS" : "FAILURES");
    return g_fail == 0 ? 0 : 1;
}
