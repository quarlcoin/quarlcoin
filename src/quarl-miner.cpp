// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.
//
// quarl-miner -- a CPU miner, and deliberately only that.
//
// The difficulty this chain opens at is set so a CPU is a reasonable thing to
// mine with, which is why this exists and why there is no GPU counterpart in
// the tree. A GPU kernel is a different kind of program with a different set of
// build dependencies, and whoever wants one can write it against the same two
// RPCs this uses. Shipping a slow official GPU miner would be worse than
// shipping none: it would set a floor nobody has any reason to beat.
//
// The whole of the protocol is two calls:
//
//   getblocktemplate   what to mine on -- the parent, the target, the
//                      transactions, and the exact witness commitment the
//                      coinbase must carry.
//   submitblock        the answer.
//
// Everything between them is assembling a coinbase and turning the nonce. The
// hash is BLAKE3 over the 80-byte header, taken twice -- once for the block
// hash and once more for the proof of work -- which is what GetPoWHash does and
// what the node will check.
//
// The RPC client here is written out rather than shared with quarl-cli. It is
// about a hundred lines of loopback HTTP, and pulling quarl-cli's apart to
// share them would put a mining program's needs into the wallet's command-line
// tool. Loopback, one connection at a time, no TLS: this talks to a node on the
// same machine or over an operator's own tunnel, and it says so in the help.

#include <arith_uint256.h>
#include <chainparams.h>
#include <chainparamsbase.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/system.h>
#include <compat/compat.h>
#include <consensus/merkle.h>
#include <core_io.h>
#include <key_io.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <pow.h>
#include <script/script.h>
#include <streams.h>
#include <univalue.h>
#include <util/exception.h>
#include <util/fs.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

// The sockets come through compat/compat.h, which is already included above:
// on Windows it pulls Winsock and on everything else the POSIX headers, and it
// defines SOCKET, INVALID_SOCKET and CloseSocket so that the code below does not
// have to know which platform it is on. Including netinet/in.h directly is what
// made this file the one thing in the tree that would not cross-compile.

const TranslateFn G_TRANSLATION_FUN{nullptr};

namespace {
/** The three socket calls that differ between Windows and everything else.
 *
 *  compat/compat.h already gives the types and the headers; what it does not
 *  give is a way to close a socket or to send on one without knowing which
 *  platform is underneath. Winsock wants closesocket() and a char* buffer,
 *  POSIX wants close() and void*, and neither knows about the other. Three
 *  lines here are cheaper than an #ifdef at each of the five call sites.
 */
inline void CloseSock(SOCKET s)
{
#ifdef WIN32
    ::closesocket(s);
#else
    ::close(s);
#endif
}

inline int SendAll(SOCKET s, const char* buf, size_t len)
{
    for (size_t sent = 0; sent < len;) {
        const auto n = ::send(s, buf + sent, static_cast<int>(len - sent), 0);
        if (n <= 0) return -1;
        sent += static_cast<size_t>(n);
    }
    return 0;
}

/** Winsock needs starting once per process; POSIX needs nothing. */
struct SocketsUp {
    SocketsUp()
    {
#ifdef WIN32
        WSADATA d;
        WSAStartup(MAKEWORD(2, 2), &d);
#endif
    }
    ~SocketsUp()
    {
#ifdef WIN32
        WSACleanup();
#endif
    }
};
} // namespace

namespace {

std::atomic<bool> g_shutdown{false};

void SetupMinerArgs(ArgsManager& argsman)
{
    SetupHelpOptions(argsman);
    SetupChainParamsBaseOptions(argsman);
    argsman.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-datadir=<dir>", "Data directory of the node -- where its cookie is read from",
                   ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-address=<addr>", "Address to pay the block reward to. Required.",
                   ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-threads=<n>", "Mining threads (default: every core but one, so the machine stays usable)",
                   ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcconnect=<ip>", "Node to mine for (default: 127.0.0.1)",
                   ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcport=<port>", "RPC port of that node (default: the chain's)",
                   ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpccookiefile=<file>", "Cookie to authenticate with (default: the datadir's .cookie)",
                   ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcuser=<user>", "RPC user, if not using a cookie", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-rpcpassword=<pw>", "RPC password, if not using a cookie", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
}

/** One JSON-RPC call over loopback HTTP. Returns the `result` member. */
class RpcClient
{
    std::string m_host, m_auth;
    uint16_t m_port;

public:
    RpcClient(std::string host, uint16_t port, std::string auth)
        : m_host(std::move(host)), m_auth(std::move(auth)), m_port(port) {}

    UniValue Call(const std::string& method, const UniValue& params)
    {
        UniValue req(UniValue::VOBJ);
        req.pushKV("jsonrpc", "2.0");
        req.pushKV("id", "quarl-miner");
        req.pushKV("method", method);
        req.pushKV("params", params);
        const std::string body{req.write()};

        const std::string head =
            "POST / HTTP/1.1\r\n"
            "Host: " + m_host + "\r\n"
            "Authorization: Basic " + EncodeBase64(m_auth) + "\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + util::ToString(body.size()) + "\r\n"
            "Connection: close\r\n\r\n";

        const SOCKET fd{socket(AF_INET, SOCK_STREAM, 0)};
        if (fd == INVALID_SOCKET) throw std::runtime_error("socket() failed");

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(m_port);
        if (inet_pton(AF_INET, m_host.c_str(), &addr.sin_addr) != 1) {
            CloseSock(fd);
            throw std::runtime_error("-rpcconnect must be an IPv4 address");
        }
        if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0) {
            CloseSock(fd);
            throw std::runtime_error("cannot reach the node -- is it running, and is -rpcport right?");
        }

        const std::string out{head + body};
        if (SendAll(fd, out.data(), out.size()) != 0) {
            CloseSock(fd);
            throw std::runtime_error("send() failed");
        }

        std::string resp;
        char buf[8192];
        for (;;) {
            const auto n = ::recv(fd, buf, static_cast<int>(sizeof buf), 0);
            if (n <= 0) break;
            resp.append(buf, static_cast<size_t>(n));
        }
        CloseSock(fd);

        const size_t split{resp.find("\r\n\r\n")};
        if (split == std::string::npos) throw std::runtime_error("malformed HTTP response");
        const std::string json{resp.substr(split + 4)};

        UniValue reply;
        if (!reply.read(json)) throw std::runtime_error("node did not return JSON: " + json.substr(0, 200));
        const UniValue& err{reply.find_value("error")};
        if (!err.isNull()) throw std::runtime_error(method + ": " + err.write());
        return reply.find_value("result");
    }
};

/** The coinbase: pays the miner, carries the height, carries the commitment. */
CMutableTransaction BuildCoinbase(int height, CAmount value, const CScript& pay_to,
                                  const std::string& witness_commitment_hex,
                                  uint32_t extra_nonce)
{
    CMutableTransaction cb;
    cb.version = 1;
    cb.vin.resize(1);
    cb.vin[0].prevout.SetNull();
    // BIP34: the height first, then anything. The extra nonce lives here so the
    // search space does not run out when the 32-bit nNonce does.
    cb.vin[0].scriptSig = CScript() << height << CScriptNum(extra_nonce);
    if (height <= 16) cb.vin[0].scriptSig << OP_0;   // scriptSig must be two bytes at least
    cb.vin[0].nSequence = CTxIn::SEQUENCE_FINAL;
    cb.nLockTime = static_cast<uint32_t>(height - 1);

    cb.vout.resize(1);
    cb.vout[0].nValue = value;
    cb.vout[0].scriptPubKey = pay_to;

    if (!witness_commitment_hex.empty()) {
        const std::vector<unsigned char> script{ParseHex(witness_commitment_hex)};
        CTxOut commit;
        commit.nValue = 0;
        commit.scriptPubKey = CScript(script.begin(), script.end());
        cb.vout.push_back(commit);
        // BIP141's reserved value: the coinbase witness is one 32-byte item.
        cb.vin[0].scriptWitness.stack.assign(1, std::vector<unsigned char>(32, 0));
    }
    return cb;
}

/** Turn the nonce until the header meets the target, or the tip moves. */
bool Grind(CBlock& block, const arith_uint256& target, int threads,
           std::atomic<uint64_t>& hashes, const std::atomic<bool>& stale)
{
    std::atomic<bool> found{false};
    std::atomic<uint32_t> winner{0};

    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t) {
        pool.emplace_back([&, t] {
            CBlock local{block};
            uint64_t local_hashes{0};
            for (uint64_t n = t; n <= 0xffffffffULL; n += threads) {
                if (found.load(std::memory_order_relaxed) || stale.load(std::memory_order_relaxed)) break;
                local.nNonce = static_cast<uint32_t>(n);
                if (UintToArith256(local.GetPoWHash()) <= target) {
                    winner.store(local.nNonce);
                    found.store(true);
                    break;
                }
                if ((++local_hashes & 0xffff) == 0) {
                    hashes.fetch_add(0x10000, std::memory_order_relaxed);
                    local_hashes = 0;
                }
            }
            hashes.fetch_add(local_hashes, std::memory_order_relaxed);
        });
    }
    for (auto& th : pool) th.join();

    if (found.load()) block.nNonce = winner.load();
    return found.load();
}

int Mine(const ArgsManager& args)
{
    const std::string addr_str{args.GetArg("-address", "")};
    if (addr_str.empty()) {
        std::fprintf(stderr, "quarl-miner: -address is required -- the reward has to be paid to somebody\n");
        return EXIT_FAILURE;
    }
    const CTxDestination dest{DecodeDestination(addr_str)};
    if (!IsValidDestination(dest)) {
        std::fprintf(stderr, "quarl-miner: -address is not an address of this chain: %s\n", addr_str.c_str());
        return EXIT_FAILURE;
    }
    const CScript pay_to{GetScriptForDestination(dest)};

    const int cores{static_cast<int>(std::thread::hardware_concurrency())};
    const int threads{static_cast<int>(args.GetIntArg("-threads", std::max(1, cores - 1)))};

    // Authentication: the cookie the node writes on startup, unless told otherwise.
    std::string auth{args.GetArg("-rpcuser", "") + ":" + args.GetArg("-rpcpassword", "")};
    if (auth == ":") {
        const fs::path cookie{args.IsArgSet("-rpccookiefile")
                                  ? fs::PathFromString(args.GetArg("-rpccookiefile", ""))
                                  : args.GetDataDirNet() / ".cookie"};
        std::ifstream in{fs::PathToString(cookie)};
        if (!in.good()) {
            std::fprintf(stderr, "quarl-miner: cannot read %s -- is the node running, and is -datadir right?\n",
                         fs::PathToString(cookie).c_str());
            return EXIT_FAILURE;
        }
        std::getline(in, auth);
    }

    RpcClient rpc{args.GetArg("-rpcconnect", "127.0.0.1"),
                  static_cast<uint16_t>(args.GetIntArg("-rpcport", BaseParams().RPCPort())),
                  auth};

    std::printf("quarl-miner %s, %d threads, paying %s\n",
                FormatFullVersion().c_str(), threads, addr_str.c_str());

    uint64_t found_blocks{0};
    std::atomic<uint64_t> hashes{0};
    auto last_report = std::chrono::steady_clock::now();

    while (!g_shutdown.load()) {
        UniValue rules(UniValue::VARR);
        rules.push_back("segwit");
        UniValue tmpl_req(UniValue::VOBJ);
        tmpl_req.pushKV("rules", rules);
        UniValue params(UniValue::VARR);
        params.push_back(tmpl_req);

        UniValue tmpl;
        try {
            tmpl = rpc.Call("getblocktemplate", params);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "quarl-miner: %s\n", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        CBlock block;
        block.nVersion = tmpl.find_value("version").getInt<int>();
        block.hashPrevBlock = uint256::FromHex(tmpl.find_value("previousblockhash").get_str()).value();
        block.nTime = static_cast<uint32_t>(tmpl.find_value("curtime").getInt<int64_t>());
        block.nBits = static_cast<uint32_t>(std::stoul(tmpl.find_value("bits").get_str(), nullptr, 16));
        block.nNonce = 0;

        const int height{tmpl.find_value("height").getInt<int>()};
        const CAmount value{tmpl.find_value("coinbasevalue").getInt<int64_t>()};
        const UniValue& commit{tmpl.find_value("default_witness_commitment")};

        block.vtx.emplace_back();   // placeholder, filled per extra nonce
        for (const UniValue& tx : tmpl.find_value("transactions").getValues()) {
            CMutableTransaction m;
            if (!DecodeHexTx(m, tx.find_value("data").get_str())) {
                std::fprintf(stderr, "quarl-miner: template carried a transaction that will not decode\n");
                return EXIT_FAILURE;
            }
            block.vtx.push_back(MakeTransactionRef(std::move(m)));
        }

        arith_uint256 target;
        target.SetCompact(block.nBits);

        // Watch the tip: a block found elsewhere makes this work worthless, and
        // the sooner that is noticed the less of it is wasted.
        std::atomic<bool> stale{false};
        const std::string mining_on{tmpl.find_value("previousblockhash").get_str()};
        std::thread watcher{[&] {
            while (!stale.load() && !g_shutdown.load()) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                try {
                    const UniValue tip{rpc.Call("getbestblockhash", UniValue{UniValue::VARR})};
                    if (tip.get_str() != mining_on) stale.store(true);
                } catch (const std::exception&) { /* try again next tick */ }
            }
        }};

        bool solved{false};
        for (uint32_t extra = 0; !solved && !stale.load() && !g_shutdown.load(); ++extra) {
            block.vtx[0] = MakeTransactionRef(
                BuildCoinbase(height, value, pay_to, commit.isStr() ? commit.get_str() : "", extra));
            block.hashMerkleRoot = BlockMerkleRoot(block);
            solved = Grind(block, target, threads, hashes, stale);

            const auto now = std::chrono::steady_clock::now();
            const double secs = std::chrono::duration<double>(now - last_report).count();
            if (secs >= 10.0) {
                const double rate = hashes.exchange(0) / secs;
                std::printf("  height %d   %7.2f kH/s   blocks found %llu\n",
                            height, rate / 1000.0, static_cast<unsigned long long>(found_blocks));
                std::fflush(stdout);
                last_report = now;
            }
        }

        stale.store(true);
        watcher.join();

        if (solved) {
            DataStream ss;
            ss << TX_WITH_WITNESS(block);
            UniValue sub(UniValue::VARR);
            sub.push_back(HexStr(ss));
            try {
                const UniValue res{rpc.Call("submitblock", sub)};
                if (res.isNull()) {
                    ++found_blocks;
                    std::printf("  block %d accepted: %s\n", height, block.GetHash().ToString().c_str());
                } else {
                    std::printf("  block %d rejected: %s\n", height, res.write().c_str());
                }
            } catch (const std::exception& e) {
                std::fprintf(stderr, "quarl-miner: submitblock: %s\n", e.what());
            }
            std::fflush(stdout);
        }
    }
    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, char* argv[])
{
    SocketsUp sockets;

    ArgsManager args;
    SetupMinerArgs(args);
    std::string error;
    if (!args.ParseParameters(argc, argv, error)) {
        std::fprintf(stderr, "quarl-miner: %s\n", error.c_str());
        return EXIT_FAILURE;
    }
    if (HelpRequested(args) || args.GetBoolArg("-version", false)) {
        std::printf("%s\n\n%s", FormatFullVersion().c_str(), args.GetHelpMessage().c_str());
        return EXIT_SUCCESS;
    }
    try {
        if (!CheckDataDirOption(args)) {
            std::fprintf(stderr, "quarl-miner: -datadir does not exist\n");
            return EXIT_FAILURE;
        }
        SelectBaseParams(args.GetChainType());
        SelectParams(args.GetChainType());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "quarl-miner: %s\n", e.what());
        return EXIT_FAILURE;
    }
    try {
        return Mine(args);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "quarl-miner: %s\n", e.what());
        return EXIT_FAILURE;
    }
}
