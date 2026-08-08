# Copyright (c) 2026 The Quarlcoin developers
# See COPYING for license.

# bench/bench_quarlcoin — nanobench microbenchmarks (built on demand:
# cmake --build <dir> --target bench_quarlcoin).
add_executable(bench_quarlcoin EXCLUDE_FROM_ALL
  bench.cpp nanobench.cpp bench_quarlcoin.cpp examples.cpp
  prevector.cpp pool.cpp lockedpool.cpp strencodings.cpp
  parse_hex.cpp random.cpp poly1305.cpp chacha20.cpp
  bech32.cpp merkle_root.cpp rollingbloom.cpp obfuscation.cpp
  hashpadding.cpp crypto_hash.cpp addrman.cpp checkqueue.cpp
  gcs_filter.cpp txgraph.cpp blockencodings.cpp ccoins_caching.cpp
  checkblockindex.cpp cluster_linearize.cpp disconnected_transactions.cpp
  duplicate_inputs.cpp index_blockfilter.cpp logging.cpp
  mempool_ephemeral_spends.cpp mempool_eviction.cpp mempool_stress.cpp
  peer_eviction.cpp rpc_mempool.cpp streams_findbyte.cpp
  util_time.cpp block_assemble.cpp sign_transaction.cpp)
target_link_libraries(bench_quarlcoin PRIVATE core_interface quarlcoin_test_util quarlcoin_node quarlcoin_wallet)
add_windows_resources(bench_quarlcoin bench_quarlcoin.rc)
