<div align="center">
  <img src="src/qt/res/icons/quarlcoin.png" alt="Quarlcoin" width="120" />

  <h1>Quarlcoin</h1>

  <p><strong>A UTXO chain on BLAKE3, with the difficulty set on every block.</strong></p>

  [![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](COPYING)
</div>

---

> **Status: launched, and young.** The genesis blocks are mined and a seed node
> is running. The chain has no history to speak of, no second implementation,
> and no security audit. What is missing is listed at the bottom of this page
> rather than left for a reader to find out the hard way.

Quarlcoin is Bitcoin Core's architecture with three things changed, each for a
reason that can be checked rather than argued about.

## What is different

**BLAKE3 instead of SHA-256, once instead of twice.** Bitcoin hashes twice
because SHA-256 is a Merkle–Damgård construction and leaks a length extension:
given `H(m)` and the length of `m`, anyone can compute `H(m ‖ padding ‖ suffix)`
without ever seeing `m`. The second pass repairs that, at the cost of doing the
work twice. BLAKE3 is a tree over a keyed permutation with a finalised root, so
there is nothing for a second pass to repair. The block hash, the merkle root,
every txid and wtxid, the sighash, the address hash and the base58 checksum are
one BLAKE3 each.

The address hash is the same root read out to twenty bytes rather than
thirty-two, so `HASH160` is a length and not a composition of two functions.
RIPEMD-160 survives only as the `OP_RIPEMD160` opcode.

**ASERT instead of a retarget epoch.** There are no difficulty periods. The
target of a block is an exponential in how far its parent is off schedule,
measured from a single anchor — the genesis block — so every target the chain
will ever have is a function of three numbers, and a node can compute the
difficulty at any height without walking the chain to it. Hashrate that arrives
or leaves is answered on the next block rather than up to two weeks later.

**Two and a half minute blocks, 84 million coins.** 50 QRL a block, halving
every 840,000 blocks, which is the same four years Bitcoin halves on.

## What is the same, deliberately

The UTXO model, the script system, SegWit and Taproot, descriptor wallets, the
peer-to-peer protocol, the mempool, the block database, the RPC interface — all
of it is Bitcoin Core's, because all of it works and none of it was the thing
worth changing.

Signatures are secp256k1: ECDSA for SegWit v0 and Schnorr (BIP 340) for Taproot,
computed by libsecp256k1. The library is carried as a subtree with one change:
its SHA-256 is BLAKE3, so there is no SHA-256 anywhere in the system. The rename
of `secp256k1_sha256` to `secp256k1_hash256` throughout that subtree is part of
that change — a struct named for a hash it does not compute is how a reader
comes to a wrong conclusion about consensus code.

## Networks

| | main | test | regtest |
|---|---|---|---|
| P2P port | 9743 | 19743 | 19844 |
| RPC port | 9742 | 19742 | 19843 |
| address prefix | `Q…` `R…` `qrl1…` | `m…` `2…` `tqrl1…` | `m…` `2…` `qrlrt1…` |
| BIP32 keys | `43q…` | `42S…` | `42S…` |
| BIP44 coin type | 9743 | 1 | 1 |

None of these values is shared with Bitcoin, Litecoin or Dogecoin. The BIP32
version bytes and the coin type in particular were checked against those chains
and against the SLIP-0044 registry: a wallet or a block explorer shown a
Quarlcoin key must not report a Bitcoin address, and the way that goes wrong is
by inheriting a constant rather than choosing one.

## Building

```sh
cmake -B build
cmake --build build -j"$(nproc)"
ctest --test-dir build
```

Add `-DENABLE_GUI=ON` for `quarl-qt`. Dependencies are Boost (headers),
libevent, SQLite and Qt 6 for the GUI; `depends/` cross-compiles them if the
system has none.

## Running

```sh
quarld -daemon
quarl-cli getblockchaininfo
```

A node with no `peers.dat` finds the network through `seed.quarlcoin.org`, or
through `-addnode` with any address of a running node.

## Mining

`quarl-miner` is a CPU miner and deliberately only that:

```sh
quarl-miner -address=<your address> -threads=8
```

There is no GPU miner in this repository. The opening difficulty is set so that
a CPU is a reasonable thing to mine with, and a slow official GPU miner would
set a floor nobody has any reason to beat. The two RPCs it uses —
`getblocktemplate` and `submitblock` — are the same two anyone else's miner
would use.

## What is not here

- **No second implementation.** Every node on the network runs this code, so a
  consensus bug in it is a consensus bug in the network.
- **No security audit.** The consensus changes are covered by tests that check
  them against independent references — the ASERT target against a separate
  implementation in Python, BLAKE3 against the official vectors and its own SIMD
  kernels against its portable one — but nobody outside this project has read
  the code.
- **No hardware wallet, no PSBT tooling, no block explorer.** The signature
  scheme is Bitcoin's, but the sighash is BLAKE3, so a device that computes the
  sighash itself will not sign for this chain until it is taught to.
- **No premine and no allocation to anyone.** The genesis coinbase pays to
  `OP_RETURN`: the 50 QRL of the first block cannot be spent by anybody. Every
  coin after it is mined.

## Contact

shortvector@proton.me
