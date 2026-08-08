// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.
//
// hash.h (HashWriter / Hash, the consensus hash) tests + cross-check CLI.
// Standalone for now (own main); folds into the test framework when it lands.
//
// The consensus hash is double SHA-256 and Hash160 is RIPEMD-160 over one
// SHA-256, exactly as in Bitcoin. These checks asserted SHA3-256 until now,
// from the design where ML-DSA signatures came with a Keccak hash; the code
// never followed, and the genesis blocks were mined on SHA-256.
//
//   no args        run the built-in checks, exit non-zero on any failure
//   --hash <hex>   forward hex of Hash(ParseHex(hex))
//                  (== python sha256(sha256(bytes)).hexdigest())

#include <crypto/hex_base.h>
#include <hash.h>
#include <serialize.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>
#include <vector>

namespace {

// Forward (non-reversed) hex of a uint256's bytes == the raw digest, which is
// what python hashlib.sha256(hashlib.sha256(b).digest()).hexdigest() produces.
// uint256::GetHex reverses; this does not.
std::string FwdHex(const uint256& h)
{
    return HexStr(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(h.data()), h.size()));
}

int g_fail = 0;
void Check(const char* name, bool ok)
{
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++g_fail;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc == 3 && std::strcmp(argv[1], "--hash") == 0) {
        std::printf("%s\n", FwdHex(Hash(ParseHex<unsigned char>(argv[2]))).c_str());
        return 0;
    }

    // Hash() is BLAKE3, once. It was SHA-256 applied twice, and the second pass
    // was there to close SHA-256's length extension; BLAKE3 has none to close,
    // so there is one pass and these vectors are the official BLAKE3 ones.
    Check("Hash(empty)", FwdHex(Hash(std::vector<unsigned char>{})) ==
          "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262");
    {
        std::vector<unsigned char> abc{'a', 'b', 'c'};
        Check("Hash(abc)", FwdHex(Hash(abc)) ==
              "6437b3ac38465133ffb63b75273a8db548c558465d79db03fd359c6cd5bd9d85");
    }

    // Two-arg Hash(a, b) == Hash(a || b).
    {
        std::vector<unsigned char> a{1, 2, 3}, b{4, 5, 6}, ab{1, 2, 3, 4, 5, 6};
        Check("Hash(a,b)==Hash(ab)", Hash(a, b) == Hash(ab));
    }

    // HashWriter << serialized object == Hash of the serialized bytes.
    {
        HashWriter w;
        w << uint32_t{0x01020304};
        // serialized little-endian bytes of 0x01020304 are {04,03,02,01}
        std::vector<unsigned char> le{0x04, 0x03, 0x02, 0x01};
        Check("HashWriter << u32 == Hash(LE bytes)", w.GetHash() == Hash(le));
    }

    // HashWriter on a string matches Hash of its raw bytes (no length prefix via write()).
    {
        std::vector<unsigned char> bytes{'t', 'e', 's', 't'};
        HashWriter w;
        w.write(std::span<const std::byte>(reinterpret_cast<const std::byte*>(bytes.data()), bytes.size()));
        Check("HashWriter.write == Hash", w.GetHash() == Hash(bytes));
    }

    // GetCheapHash is the first 64 bits (LE) of the hash.
    {
        std::vector<unsigned char> data{9, 8, 7, 6, 5};
        uint256 h = Hash(data);
        HashWriter w;
        w.write(std::span<const std::byte>(reinterpret_cast<const std::byte*>(data.data()), data.size()));
        uint64_t cheap = w.GetCheapHash();
        Check("GetCheapHash==ReadLE64(hash)", cheap == h.GetUint64(0));
    }

    // uint256::FromHex round-trips the reverse-display GetHex of a real hash.
    {
        uint256 h = Hash(std::vector<unsigned char>{'x'});
        auto back = uint256::FromHex(h.GetHex());
        Check("GetHex/FromHex round-trip", back && *back == h);
    }

    // Hash160 = RIPEMD-160 over one SHA-256, which is a different function and
    // not a prefix of Hash(). The old check compared it against the first 20
    // bytes of the 256-bit hash, and that only ever held for a design where both
    // were the same Keccak digest truncated differently.
    {
        std::vector<unsigned char> data{'s', 'c', 'r', 'i', 'p', 't'};
        const uint160 h160 = Hash160(data);
        const std::string fwd = HexStr(std::span<const uint8_t>(
            reinterpret_cast<const uint8_t*>(h160.data()), h160.size()));
        Check("Hash160(script) KAT", fwd == "6c737280e30ad499f0612336da629536d6af3f7b");
        // And the relationship is now the opposite of what it was. Hash160 used
        // to be RIPEMD-160 over a SHA-256, two functions composed, so its output
        // had nothing to do with the first twenty bytes of Hash(). It is now the
        // same BLAKE3 root read out to twenty bytes instead of thirty-two, which
        // makes an address hash a shorter read of the transaction hash rather
        // than a second computation -- and this is the test that says so.
        Check("Hash160 == Hash[:20]",
              std::memcmp(h160.data(), Hash(data).data(), 20) == 0);
    }

    std::printf("%s\n", g_fail == 0 ? "ALL PASS" : "FAILURES");
    return g_fail == 0 ? 0 : 1;
}
