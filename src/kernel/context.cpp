// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#include <kernel/context.h>

#include <crypto/sha256.h>
#include <logging.h>
#include <random.h>

#include <mutex>
#include <string>

namespace kernel {
Context::Context()
{
    static std::once_flag globals_initialized{};
    std::call_once(globals_initialized, []() {
        // Pick the SHA-256 implementation before anything hashes. This was
        // skipped on the reasoning that the consensus hash was SHA3 and SHA-256
        // had only one implementation; both halves were wrong. Every txid, every
        // Merkle fold and every proof-of-work check is SHA-256, and without this
        // call they all run the portable C fallback while the SHA-NI and AVX2
        // paths sit compiled and unreachable.
        //
        // The elliptic-curve context is not started here. It is per-process
        // state with a destructor, so it belongs to whoever owns the process:
        // the daemon, the GUI's node, and each test's main.
        const std::string sha256_algo = SHA256AutoDetect();
        LogDebug(BCLog::VALIDATION, "Using the '%s' SHA256 implementation\n", sha256_algo);
        RandomInit();
    });
}
} // namespace kernel
