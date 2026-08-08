// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#include <common/signmessage.h>

#include <chainparams.h>
#include <hash.h>
#include <key.h>
#include <key_io.h>
#include <pubkey.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <cassert>
#include <optional>
#include <string>
#include <variant>
#include <vector>

/**
 * Text used to signify that a signed message follows and to prevent
 * inadvertently signing a transaction.
 */
const std::string MESSAGE_MAGIC = "Quarlcoin Signed Message:\n";

MessageVerificationResult MessageVerify(
    const std::string& address,
    const std::string& signature,
    const std::string& message)
{
    CTxDestination destination = DecodeDestination(address);
    if (!IsValidDestination(destination)) {
        return MessageVerificationResult::ERR_INVALID_ADDRESS;
    }

    // Addresses here are witness-only, so the signer is named by a
    // WitnessV0KeyHash rather than the legacy base58 PKHash Core matches on.
    if (std::get_if<WitnessV0KeyHash>(&destination) == nullptr) {
        return MessageVerificationResult::ERR_ADDRESS_NO_KEY;
    }

    auto signature_bytes = DecodeBase64(signature);
    if (!signature_bytes) {
        return MessageVerificationResult::ERR_MALFORMED_SIGNATURE;
    }

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(MessageHash(message), *signature_bytes)) {
        return MessageVerificationResult::ERR_PUBKEY_NOT_RECOVERED;
    }

    if (!(WitnessV0KeyHash(pubkey) == *std::get_if<WitnessV0KeyHash>(&destination))) {
        return MessageVerificationResult::ERR_NOT_SIGNED;
    }

    return MessageVerificationResult::OK;
}

bool MessageSign(
    const CKey& privkey,
    const std::string& message,
    std::string& signature)
{
    std::vector<unsigned char> signature_bytes;

    if (!privkey.SignCompact(MessageHash(message), signature_bytes)) {
        return false;
    }

    signature = EncodeBase64(signature_bytes);

    return true;
}

uint256 MessageHash(const std::string& message)
{
    HashWriter hasher{};
    hasher << MESSAGE_MAGIC << message;

    return hasher.GetHash();
}

std::string SigningResultString(const SigningResult res)
{
    switch (res) {
        case SigningResult::OK:
            return "No error";
        case SigningResult::PRIVATE_KEY_NOT_AVAILABLE:
            return "Private key not available";
        case SigningResult::SIGNING_FAILED:
            return "Sign failed";
        // no default case, so the compiler can warn about missing cases
    }
    assert(false);
}
