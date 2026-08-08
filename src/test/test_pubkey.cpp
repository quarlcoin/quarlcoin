// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.
//
// Tests for CPubKey over secp256k1: the two encodings and their sizes, validity
// by length and by whether the bytes are a point on the curve, end-to-end
// sign/verify (with tampered message / signature / key rejected), recovery from
// a compact signature, and GetID = Hash160(pubkey) cross-checked against an
// independent digest.
//
// It tested a crypto-agile CPubKey over a vendored ML-DSA-44 before, down to the
// scheme byte in front of the key. There is no scheme byte and no second scheme:
// a key is a point, its length says which encoding, and the questions worth
// asking changed with it.

#include <hash.h>
#include <key.h>
#include <pubkey.h>
#include <uint256.h>

#include <cstdint>
#include <cstdio>
#include <span>
#include <algorithm>
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
    ECC_Context ecc_context{};

    // The two encodings. Uncompressed carries both coordinates; compressed
    // carries x and the parity of y, which is all the curve equation needs.
    Check("SIZE == 65", CPubKey::SIZE == 65);
    Check("COMPRESSED_SIZE == 33", CPubKey::COMPRESSED_SIZE == 33);
    Check("COMPACT_SIGNATURE_SIZE == 65", CPubKey::COMPACT_SIGNATURE_SIZE == 65);

    const CKey key{GenerateRandomKey()};
    Check("a generated key is valid", key.IsValid());
    const CPubKey pub{key.GetPubKey()};
    Check("its public key is 33 bytes", pub.size() == CPubKey::COMPRESSED_SIZE);
    Check("and compressed", pub.IsCompressed());
    Check("and a point on the curve", pub.IsFullyValid());

    // Length decides the encoding, and a length no encoding uses decides
    // nothing: such a key is invalid before anything is verified against it.
    {
        std::vector<unsigned char> raw(pub.begin(), pub.end());
        raw.pop_back();
        CPubKey truncated;
        truncated.Set(raw.begin(), raw.end());
        Check("a key one byte short is invalid", !truncated.IsValid());
        Check("and ValidSize refuses it", !CPubKey::ValidSize(raw));
    }

    // The right length and the wrong point. This is the check a length test
    // cannot make: 33 bytes that are not a point still parse and must not
    // verify.
    //
    // The x here is all ones, which is larger than the field prime
    // 2^256 - 2^32 - 977 and so is not a coordinate at all. Flipping a byte of
    // a real x would be the obvious thing and is wrong: about half of all x
    // values do have a square root, so that test passes or fails depending on
    // which key was generated.
    {
        std::vector<unsigned char> raw(pub.begin(), pub.end());
        std::fill(raw.begin() + 1, raw.end(), 0xff);
        CPubKey bent;
        bent.Set(raw.begin(), raw.end());
        Check("a key of the right length off the curve is not fully valid", !bent.IsFullyValid());
    }

    // Sign a sighash-sized message and verify through CPubKey.
    uint256 msg;
    for (size_t i = 0; i < uint256::size(); ++i) msg.begin()[i] = (unsigned char)(i * 7 + 1);
    std::vector<unsigned char> sig;
    Check("sign", key.Sign(msg, sig));
    Check("the signature is DER, at most 72 bytes", !sig.empty() && sig.size() <= CPubKey::SIGNATURE_SIZE);
    Check("Verify valid", pub.Verify(msg, sig));
    Check("and its S is the low one of the pair", CPubKey::CheckLowS(sig));

    {
        uint256 bad = msg;
        bad.begin()[0] ^= 0x01;
        Check("Verify rejects tampered message", !pub.Verify(bad, sig));
    }
    {
        std::vector<unsigned char> bad = sig;
        bad[10] ^= 0x80;
        Check("Verify rejects tampered signature", !pub.Verify(msg, bad));
    }
    {
        const CPubKey other{GenerateRandomKey().GetPubKey()};
        Check("Verify rejects wrong key", !other.Verify(msg, sig));
    }
    {
        CPubKey nothing;
        Check("an invalid key never verifies", !nothing.Verify(msg, sig));
    }

    // The compact form, which is what a training record carries: 65 fixed bytes
    // the signer's key can be recovered from, so the record does not have to
    // carry the key twice.
    {
        std::vector<unsigned char> compact;
        Check("sign compact", key.SignCompact(msg, compact));
        Check("compact is exactly 65 bytes", compact.size() == CPubKey::COMPACT_SIGNATURE_SIZE);
        CPubKey recovered;
        Check("recover", recovered.RecoverCompact(msg, compact));
        Check("and what comes back is the key that signed", recovered == pub);

        CPubKey wrong;
        uint256 other_msg = msg;
        other_msg.begin()[31] ^= 0x01;
        Check("recovering against another message gives another key or none",
              !wrong.RecoverCompact(other_msg, compact) || wrong != pub);
    }

    // The uncompressed encoding of the same point is a different 65 bytes and a
    // different identity, which is why an output type has to fix one of them.
    {
        CPubKey grown{pub};
        Check("decompress", grown.Decompress());
        Check("gives 65 bytes", grown.size() == CPubKey::SIZE);
        Check("still on the curve", grown.IsFullyValid());
        Check("still verifies the same signature", grown.Verify(msg, sig));
        Check("and hashes to a different identity", grown.GetID() != pub.GetID());
    }

    // GetID = Hash160(pubkey), checked against an independent digest rather than
    // against the same function under another name.
    {
        const uint160 expect{Hash160(std::span<const unsigned char>(pub.data(), pub.size()))};
        Check("GetID == Hash160(pubkey)", pub.GetID() == CKeyID{expect});
    }

    std::printf("%s\n", g_fail == 0 ? "ALL PASS" : "FAILURES");
    return g_fail == 0 ? 0 : 1;
}
