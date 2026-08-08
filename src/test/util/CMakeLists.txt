# Copyright (c) 2026 The Quarlcoin developers
# See COPYING for license.

# test/util — Core-style test fixtures (TestingSetup / TestChain100Setup), built
# on demand for the node-context benches and fixture tests.
add_library(quarlcoin_test_util STATIC EXCLUDE_FROM_ALL
  blockfilter.cpp
  coins.cpp
  coverage.cpp
  json.cpp
  mining.cpp
  net.cpp
  random.cpp
  script.cpp
  setup_common.cpp
  transaction_utils.cpp
  txmempool.cpp
  validation.cpp
)
target_link_libraries(quarlcoin_test_util PUBLIC core_interface quarlcoin_node quarlcoin_wallet)
