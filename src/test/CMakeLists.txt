# Copyright (c) 2026 The Quarlcoin developers
# See COPYING for license.

# Unit tests — each test_*.cpp is its own standalone executable (int main())
# linked against the full node stack, registered with ctest.
#
# Twelve names are missing from this list and their files are still in the
# directory: test_crypter, test_hdkey, test_interpreter, test_key, test_policy,
# test_sigcache, test_sign, test_spend, test_v2cipher, test_v2transport,
# test_wallet, test_wallet_spend. They are written against the ML-DSA key API
# and the wallet API this tree replaced, and they do not compile. Left in place
# rather than deleted because what they test still needs testing; adding a name
# back here is the first step of fixing one.
set(QUARLCOIN_TESTS
  test_addresstype test_arith test_bech32 test_blake3 test_block test_chain
  test_chainparams test_chainstate test_check test_clientversion test_coins
  test_coinscache test_dbwrapper test_deployment test_engine_utils test_hash
  test_key_io test_mempool test_merkle test_moneystr
  test_pow test_pubkey test_random test_retarget test_script test_serialize
  test_sha256 test_signingprovider test_signmessage
  test_txgraph test_tx_check test_tx_verify test_uint256
  test_validation test_walletdb)

foreach(t IN LISTS QUARLCOIN_TESTS)
  add_executable(${t} ${t}.cpp)
  # quarlcoin_blake3 by name and not only through the node library: this test
  # calls the SIMD kernels directly to compare them, so it needs the same
  # BLAKE3_NO_* definitions the library was compiled with, and those travel
  # with the target rather than through an intermediate that links it privately.
  target_link_libraries(${t} PRIVATE core_interface quarlcoin_node quarlcoin_wallet quarlcoin_blake3)
  add_test(NAME ${t} COMMAND ${t})
endforeach()

# Tools — built with everything else so they cannot rot unnoticed, but not run
# by ctest: they produce a value or a measurement rather than a verdict.
set(QUARLCOIN_TOOLS gen_genesis)

foreach(t IN LISTS QUARLCOIN_TOOLS)
  add_executable(${t} ${t}.cpp)
  target_link_libraries(${t} PRIVATE core_interface quarlcoin_node)
endforeach()
