// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.
//
// Round-trip test for the signed-message helpers. Bitcoin's MessageSign/
// MessageVerify use ECDSA recoverable signatures (the signer's key is recovered from the
// signature); this carried no recovery when signatures were ML-DSA, so a Quarlcoin signed message carries the public
// key followed by the signature, and MessageVerify checks the embedded key against the address.

#include <common/signmessage.h>

#include <chainparams.h>
#include <key.h>
#include <key_io.h>
#include <pubkey.h>
#include <util/chaintype.h>

#include <cstdio>
#include <string>

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

    SelectParams(ChainType::REGTEST);

    CKey key;
    key.MakeNewKey(/*fCompressed=*/true);
    const CPubKey pubkey = key.GetPubKey();
    const std::string address = EncodeDestination(WitnessV0KeyHash(pubkey));
    const std::string message = "Quarlcoin signed message round-trip";

    std::string signature;
    Check("MessageSign succeeds", MessageSign(key, message, signature));
    Check("signature is non-empty", !signature.empty());

    Check("MessageVerify accepts a valid signature",
          MessageVerify(address, signature, message) == MessageVerificationResult::OK);

    Check("MessageVerify rejects a tampered message",
          MessageVerify(address, signature, message + "!") != MessageVerificationResult::OK);

    // A signature made by a different key must not verify against this address.
    {
        CKey other;
        other.MakeNewKey(/*fCompressed=*/true);
        std::string other_sig;
        Check("sign with a second key", MessageSign(other, message, other_sig));
        Check("MessageVerify rejects a foreign key's signature",
              MessageVerify(address, other_sig, message) == MessageVerificationResult::ERR_NOT_SIGNED);
    }

    // Garbage / malformed inputs are rejected, not crashed on.
    Check("MessageVerify rejects an invalid address",
          MessageVerify("not-an-address", signature, message) == MessageVerificationResult::ERR_INVALID_ADDRESS);
    Check("MessageVerify rejects malformed base64",
          MessageVerify(address, "!!!not base64!!!", message) == MessageVerificationResult::ERR_MALFORMED_SIGNATURE);

    std::printf(g_fail ? "\nFAILED (%d)\n" : "\nOK\n", g_fail);
    return g_fail ? 1 : 0;
}
