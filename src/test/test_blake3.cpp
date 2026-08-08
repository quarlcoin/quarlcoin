// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.
//
// Known-answer test for BLAKE3, and a check that the SIMD kernels agree with
// the portable one.
//
// The second part is the one that matters here. Every hash this chain computes
// goes through this primitive -- the block hash, the merkle root, every txid
// and wtxid -- and blake3_dispatch.c picks a kernel at runtime from what the
// CPU reports. If an AVX2 machine and an SSE4.1 machine disagreed in a single
// bit, they would compute different block hashes from the same block and the
// chain would part in two along the line of who owns which processor. So the
// kernels are compared against each other directly, on the same inputs, rather
// than each being trusted separately.
//
// The vectors are the official ones from the BLAKE3 repository
// (test_vectors/test_vectors.json). Their input is the repeating sequence
// 0, 1, 2, ..., 250, 0, 1, ... , and the lengths were chosen upstream to land
// on either side of every boundary the implementation has: one block, one
// chunk, several chunks, and the points where the SIMD degree changes.

#include <crypto/blake3.h>
#include <crypto/blake3/blake3_impl.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {
int g_fail = 0;

std::string Hex(const unsigned char* d, size_t n)
{
    static const char* k = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; ++i) { s += k[d[i] >> 4]; s += k[d[i] & 0xf]; }
    return s;
}

/** The input the official vectors are defined over. */
std::vector<unsigned char> OfficialInput(size_t len)
{
    std::vector<unsigned char> v(len);
    for (size_t i = 0; i < len; ++i) v[i] = static_cast<unsigned char>(i % 251);
    return v;
}

struct Vector { size_t len; const char* hash; };

const Vector VECTORS[] = {
    {0, "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262"},
    {1, "2d3adedff11b61f14c886e35afa036736dcd87a74d27b5c1510225d0f592e213"},
    {2, "7b7015bb92cf0b318037702a6cdd81dee41224f734684c2c122cd6359cb1ee63"},
    {3, "e1be4d7a8ab5560aa4199eea339849ba8e293d55ca0a81006726d184519e647f"},
    {4, "f30f5ab28fe047904037f77b6da4fea1e27241c5d132638d8bedce9d40494f32"},
    {5, "b40b44dfd97e7a84a996a91af8b85188c66c126940ba7aad2e7ae6b385402aa2"},
    {6, "06c4e8ffb6872fad96f9aaca5eee1553eb62aed0ad7198cef42e87f6a616c844"},
    {7, "3f8770f387faad08faa9d8414e9f449ac68e6ff0417f673f602a646a891419fe"},
    {8, "2351207d04fc16ade43ccab08600939c7c1fa70a5c0aaca76063d04c3228eaeb"},
    {63, "e9bc37a594daad83be9470df7f7b3798297c3d834ce80ba85d6e207627b7db7b"},
    {64, "4eed7141ea4a5cd4b788606bd23f46e212af9cacebacdc7d1f4c6dc7f2511b98"},
    {65, "de1e5fa0be70df6d2be8fffd0e99ceaa8eb6e8c93a63f2d8d1c30ecb6b263dee"},
    {127, "d81293fda863f008c09e92fc382a81f5a0b4a1251cba1634016a0f86a6bd640d"},
    {128, "f17e570564b26578c33bb7f44643f539624b05df1a76c81f30acd548c44b45ef"},
    {129, "683aaae9f3c5ba37eaaf072aed0f9e30bac0865137bae68b1fde4ca2aebdcb12"},
    {1023, "10108970eeda3eb932baac1428c7a2163b0e924c9a9e25b35bba72b28f70bd11"},
    {1024, "42214739f095a406f3fc83deb889744ac00df831c10daa55189b5d121c855af7"},
    {1025, "d00278ae47eb27b34faecf67b4fe263f82d5412916c1ffd97c8cb7fb814b8444"},
    {2048, "e776b6028c7cd22a4d0ba182a8bf62205d2ef576467e838ed6f2529b85fba24a"},
    {2049, "5f4d72f40d7a5f82b15ca2b2e44b1de3c2ef86c426c95c1af0b6879522563030"},
    {3072, "b98cb0ff3623be03326b373de6b9095218513e64f1ee2edd2525c7ad1e5cffd2"},
    {3073, "7124b49501012f81cc7f11ca069ec9226cecb8a2c850cfe644e327d22d3e1cd3"},
    {4096, "015094013f57a5277b59d8475c0501042c0b642e531b0a1c8f58d2163229e969"},
    {4097, "9b4052b38f1c5fc8b1f9ff7ac7b27cd242487b3d890d15c96a1c25b8aa0fb995"},
    {5120, "9cadc15fed8b5d854562b26a9536d9707cadeda9b143978f319ab34230535833"},
    {5121, "628bd2cb2004694adaab7bbd778a25df25c47b9d4155a55f8fbd79f2fe154cff"},
    {6144, "3e2e5b74e048f3add6d21faab3f83aa44d3b2278afb83b80b3c35164ebeca205"},
    {6145, "f1323a8631446cc50536a9f705ee5cb619424d46887f3c376c695b70e0f0507f"},
    {7168, "61da957ec2499a95d6b8023e2b0e604ec7f6b50e80a9678b89d2628e99ada77a"},
    {7169, "a003fc7a51754a9b3c7fae0367ab3d782dccf28855a03d435f8cfe74605e7817"},
    {8192, "aae792484c8efe4f19e2ca7d371d8c467ffb10748d8a5a1ae579948f718a2a63"},
    {8193, "bab6c09cb8ce8cf459261398d2e7aef35700bf488116ceb94a36d0f5f1b7bc3b"},
    {16384, "f875d6646de28985646f34ee13be9a576fd515f76b5b0a26bb324735041ddde4"},
    {31744, "62b6960e1a44bcc1eb1a611a8d6235b6b4b78f32e7abc4fb4c6cdcce94895c47"},
};

void Check(const char* name, bool ok)
{
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++g_fail;
}

/** Run one kernel over `n` one-block inputs and return its output. */
using HashMany = void (*)(const uint8_t* const*, size_t, size_t, const uint32_t[8],
                          uint64_t, bool, uint8_t, uint8_t, uint8_t, uint8_t*);

std::vector<uint8_t> RunKernel(HashMany fn, size_t num_inputs, size_t blocks)
{
    const size_t block_len = 64;
    std::vector<std::vector<uint8_t>> storage(num_inputs);
    std::vector<const uint8_t*> ptrs(num_inputs);
    for (size_t i = 0; i < num_inputs; ++i) {
        storage[i].resize(blocks * block_len);
        for (size_t j = 0; j < storage[i].size(); ++j) {
            storage[i][j] = static_cast<uint8_t>((j * 7 + i * 31) % 251);
        }
        ptrs[i] = storage[i].data();
    }
    // An arbitrary but fixed key and counter: what is being compared is whether
    // two kernels agree, so the only requirement on these is that both see the
    // same ones.
    const uint32_t key[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                             0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    std::vector<uint8_t> out(num_inputs * 32, 0);
    fn(ptrs.data(), num_inputs, blocks, key, /*counter=*/11, /*increment_counter=*/true,
       /*flags=*/0, /*flags_start=*/1, /*flags_end=*/2, out.data());
    return out;
}

void CompareKernel(const char* name, HashMany fn)
{
    bool ok = true;
    for (size_t num_inputs : {size_t{1}, size_t{2}, size_t{4}, size_t{7}, size_t{8}, size_t{16}}) {
        for (size_t blocks : {size_t{1}, size_t{2}, size_t{16}}) {
            if (RunKernel(fn, num_inputs, blocks) !=
                RunKernel(blake3_hash_many_portable, num_inputs, blocks)) {
                ok = false;
            }
        }
    }
    Check(name, ok);
}
} // namespace

int main()
{
    std::printf("-- official vectors, through the dispatcher --\n");
    for (const Vector& v : VECTORS) {
        const std::vector<unsigned char> in = OfficialInput(v.len);
        unsigned char out[CBLAKE3::OUTPUT_SIZE];
        CBLAKE3().Write(in.data(), in.size()).Finalize(out);
        const std::string got = Hex(out, sizeof(out));
        char name[64];
        std::snprintf(name, sizeof(name), "len %zu", v.len);
        const bool ok = got == v.hash;
        std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
        if (!ok) {
            std::printf("      got %s\n      exp %s\n", got.c_str(), v.hash);
            ++g_fail;
        }
    }

    std::printf("\n-- the same, fed one byte at a time --\n");
    {
        // Writing in pieces must not change the answer: a transaction is
        // serialised into the hasher field by field, never in one call.
        bool ok = true;
        for (const Vector& v : VECTORS) {
            if (v.len > 8192) continue;
            const std::vector<unsigned char> in = OfficialInput(v.len);
            CBLAKE3 h;
            for (unsigned char c : in) h.Write(&c, 1);
            unsigned char out[CBLAKE3::OUTPUT_SIZE];
            h.Finalize(out);
            if (Hex(out, sizeof(out)) != v.hash) ok = false;
        }
        Check("byte-at-a-time agrees with one call", ok);
    }

    std::printf("\n-- the 20-byte output is a prefix of the 32-byte one --\n");
    {
        // What CHash160 relies on: an address hash is a shorter read of the
        // same root, not a different function.
        bool ok = true;
        for (const Vector& v : VECTORS) {
            const std::vector<unsigned char> in = OfficialInput(v.len);
            unsigned char full[32], twenty[20];
            CBLAKE3().Write(in.data(), in.size()).Finalize(full);
            CBLAKE3 h;
            h.Write(in.data(), in.size());
            h.FinalizeXOF(twenty);
            if (std::memcmp(full, twenty, 20) != 0) ok = false;
        }
        Check("20 bytes == first 20 of 32", ok);
    }

    std::printf("\n-- the SIMD kernels against the portable one --\n");
    std::printf("     (a disagreement here is a chain split along processor lines)\n");
#if defined(BLAKE3_NO_SSE2)
    std::printf("[skip] sse2 not compiled in\n");
#else
    CompareKernel("sse2 == portable", blake3_hash_many_sse2);
#endif
#if defined(BLAKE3_NO_SSE41)
    std::printf("[skip] sse41 not compiled in\n");
#else
    CompareKernel("sse41 == portable", blake3_hash_many_sse41);
#endif
#if defined(BLAKE3_NO_AVX2)
    std::printf("[skip] avx2 not compiled in\n");
#else
    CompareKernel("avx2 == portable", blake3_hash_many_avx2);
#endif
#if defined(BLAKE3_NO_AVX512)
    std::printf("[skip] avx512 not compiled in\n");
#else
    CompareKernel("avx512 == portable", blake3_hash_many_avx512);
#endif

    std::printf(g_fail ? "\nFAILED (%d)\n" : "\nOK\n", g_fail);
    return g_fail ? 1 : 0;
}
