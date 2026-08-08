# Copyright (c) 2026 The Quarlcoin developers
# See COPYING for license.
#
# Library decomposition mirroring Bitcoin Core's src/CMakeLists.txt: the former
# monolithic quarlcoin_core is split into a clean dependency DAG of static libs
#   crypto  <-  util  <-  consensus  <-  common  <-  node
#                                   wallet  <-  common
#                                   cli (standalone)
# All carry the shared flags/includes via the core_interface INTERFACE library
# (defined in the top-level CMakeLists). The build config header is generated
# next to this file.

include(AddWindowsResources)
include(InstallBinaryComponent)

# univalue — the JSON library (vendored under src/univalue), used by every layer
# that speaks RPC/JSON.
add_subdirectory(univalue)

# libsecp256k1 — the consensus signature library, carried as a subtree exactly
# as Bitcoin Core carries it, and wired up by Core's own cmake module so the
# build options (which modules, which flags) are the ones upstream tests.
include(../cmake/secp256k1.cmake)
add_secp256k1(secp256k1)
# The subtree hashes with BLAKE3 since its SHA-256 was replaced (see the note in
# secp256k1/src/hash.h). quarlcoin_blake3 is defined in crypto/ below; CMake
# resolves the dependency at generate time, so the order here does not matter.
target_link_libraries(secp256k1 PRIVATE quarlcoin_blake3)

# clientversion — depends on the generated quarlcoin-build-info.h.
add_library(quarlcoin_clientversion STATIC EXCLUDE_FROM_ALL
  clientversion.cpp
)
target_link_libraries(quarlcoin_clientversion PRIVATE core_interface)
add_dependencies(quarlcoin_clientversion generate_build_info)

add_subdirectory(crypto)
add_subdirectory(util)

# Consensus-critical code (block/tx validation primitives, script interpreter).
add_library(quarlcoin_consensus STATIC EXCLUDE_FROM_ALL
  arith_uint256.cpp
  consensus/merkle.cpp
  consensus/tx_check.cpp
  hash.cpp
  primitives/block.cpp
  primitives/transaction.cpp
  pubkey.cpp
  script/interpreter.cpp
  script/script.cpp
  script/script_error.cpp
  uint256.cpp
)
target_link_libraries(quarlcoin_consensus
  PRIVATE
    core_interface
    quarlcoin_crypto
  PUBLIC
    # pubkey.cpp verifies secp256k1 signatures in CHECKSIG.
    secp256k1
)

if(WITH_ZMQ)
  add_subdirectory(zmq)
endif()

# Higher-level functionality shared by the executables and libraries.
add_library(quarlcoin_common STATIC EXCLUDE_FROM_ALL
  addresstype.cpp
  base58.cpp
  bech32.cpp
  chain.cpp
  chainparams.cpp
  chainparamsbase.cpp
  coins.cpp
  common/args.cpp
  common/bloom.cpp
  common/config.cpp
  common/init.cpp
  common/interfaces.cpp
  common/license_info.cpp
  common/messages.cpp
  common/netif.cpp
  common/run_command.cpp
  common/settings.cpp
  common/signmessage.cpp
  common/system.cpp
  common/url.cpp
  compressor.cpp
  core_io.cpp
  deploymentinfo.cpp
  external_signer.cpp
  init/common.cpp
  kernel/chainparams.cpp
  key.cpp
  key_io.cpp
  merkleblock.cpp
  musig.cpp
  net_permissions.cpp
  net_types.cpp
  netaddress.cpp
  netbase.cpp
  outputtype.cpp
  policy/feerate.cpp
  policy/policy.cpp
  pow.cpp
  protocol.cpp
  psbt.cpp
  rpc/rawtransaction_util.cpp
  rpc/request.cpp
  rpc/util.cpp
  scheduler.cpp
  script/descriptor.cpp
  script/miniscript.cpp
  script/parsing.cpp
  script/sign.cpp
  script/signingprovider.cpp
  script/solver.cpp
)
# Boost::headers because common/interfaces.cpp includes boost/signals2. It was
# missing and the native build did not care: the system boost lives in
# /usr/include, which every compiler searches anyway. A cross build searches
# only what it is told, and this is where it stopped.
target_link_libraries(quarlcoin_common
  PRIVATE
    core_interface
    quarlcoin_consensus
    quarlcoin_util
    univalue
    Boost::headers
  PUBLIC
    $<$<PLATFORM_ID:Windows>:ws2_32>
)

if(ENABLE_WALLET)
  add_subdirectory(wallet)

  # quarl-wallet — offline wallet tool (no running node).
  add_executable(quarl-wallet
    quarl-wallet.cpp
    wallet/wallettool.cpp
  )
  target_link_libraries(quarl-wallet
    core_interface
    quarlcoin_wallet
    quarlcoin_common
    quarlcoin_util
  )
endif()

# P2P and RPC server functionality used by quarld and quarl-qt.
add_library(quarlcoin_node STATIC EXCLUDE_FROM_ALL
  addrdb.cpp
  addrman.cpp
  banman.cpp
  blockencodings.cpp
  blockfilter.cpp
  consensus/tx_verify.cpp
  dbwrapper.cpp
  deploymentstatus.cpp
  flatfile.cpp
  headerssync.cpp
  httprpc.cpp
  httpserver.cpp
  i2p.cpp
  index/base.cpp
  index/blockfilterindex.cpp
  index/coinstatsindex.cpp
  index/txindex.cpp
  index/txospenderindex.cpp
  init.cpp
  kernel/chain.cpp
  kernel/checks.cpp
  kernel/coinstats.cpp
  kernel/context.cpp
  kernel/cs_main.cpp
  kernel/disconnected_transactions.cpp
  kernel/mempool_removal_reason.cpp
  mapport.cpp
  net.cpp
  net_processing.cpp
  netgroup.cpp
  node/abort.cpp
  node/blockmanager_args.cpp
  node/blockstorage.cpp
  node/caches.cpp
  node/chainstate.cpp
  node/chainstatemanager_args.cpp
  node/coin.cpp
  node/coins_view_args.cpp
  node/connection_types.cpp
  node/context.cpp
  node/database_args.cpp
  node/eviction.cpp
  node/interface_ui.cpp
  node/interfaces.cpp
  node/psbt.cpp
  node/kernel_notifications.cpp
  node/mempool_args.cpp
  node/mempool_persist.cpp
  node/mempool_persist_args.cpp
  node/mini_miner.cpp
  node/miner.cpp
  node/mining_args.cpp
  node/peerman_args.cpp
  node/timeoffsets.cpp
  node/transaction.cpp
  node/txdownloadman_impl.cpp
  node/txorphanage.cpp
  node/txreconciliation.cpp
  node/utxo_snapshot.cpp
  node/warnings.cpp
  noui.cpp
  policy/ephemeral_policy.cpp
  policy/fees/block_policy_estimator.cpp
  policy/fees/block_policy_estimator_args.cpp
  policy/packages.cpp
  policy/rbf.cpp
  policy/settings.cpp
  policy/truc_policy.cpp
  private_broadcast.cpp
  rest.cpp
  rpc/blockchain.cpp
  rpc/fees.cpp
  rpc/mempool.cpp
  rpc/mining.cpp
  rpc/net.cpp
  rpc/node.cpp
  rpc/output_script.cpp
  rpc/rawtransaction.cpp
  rpc/server.cpp
  rpc/server_util.cpp
  rpc/signmessage.cpp
  rpc/txoutproof.cpp
  script/sigcache.cpp
  torcontrol.cpp
  txdb.cpp
  txgraph.cpp
  txmempool.cpp
  txrequest.cpp
  bip324.cpp
  validation.cpp
  validationinterface.cpp
  versionbits.cpp
  $<$<TARGET_EXISTS:quarlcoin_wallet>:wallet/init.cpp>
)
target_link_libraries(quarlcoin_node
  PRIVATE
    core_interface
    quarlcoin_common
    quarlcoin_util
    $<TARGET_NAME_IF_EXISTS:quarlcoin_zmq>
    leveldb
    minisketch
    univalue
  PUBLIC
    # PUBLIC and not PRIVATE: txmempool.h includes boost/multi_index and reaches
    # every executable through interfaces/chain.h -> policy/rbf.h. Kept private,
    # the include path stopped at this library and each binary found boost in
    # /usr/include instead -- which is not a path a cross build has.
    Boost::headers
)

# quarld — the headless node daemon. Links its own MakeNodeInit
# (init/quarld.cpp, no IPC); the IPC quarl-node below links a different one.
add_executable(quarld quarld.cpp init/quarld.cpp)
add_windows_resources(quarld quarld.rc)
target_link_libraries(quarld
  core_interface
  quarlcoin_node
  $<TARGET_NAME_IF_EXISTS:quarlcoin_wallet>
)

# Multiprocess IPC (unix only). The quarl-node binary is quarld with the
# IPC-enabled MakeNodeInit; the stub MakeIpc is folded into the node lib otherwise.
if(ENABLE_IPC)
  # Build the vendored libmultiprocess (defines the Libmultiprocess::multiprocess
  # and ::mpgen targets and the MP_INCLUDE_DIR property), then load its
  # target_capnp_sources() helper. The helper is shipped for downstream
  # find_package() consumers, so for this vendored (add_subdirectory) build it
  # must be include()'d explicitly before src/ipc uses it.
  add_subdirectory(ipc/libmultiprocess EXCLUDE_FROM_ALL)
  include(${CMAKE_CURRENT_SOURCE_DIR}/ipc/libmultiprocess/cmake/TargetCapnpSources.cmake)
  add_subdirectory(ipc)
  add_executable(quarl-node quarld.cpp init/quarl-node.cpp)
  target_link_libraries(quarl-node core_interface quarlcoin_node quarlcoin_ipc $<TARGET_NAME_IF_EXISTS:quarlcoin_wallet>)
  add_executable(ipc_spawn_test EXCLUDE_FROM_ALL ipc/test/spawn_test.cpp)
  add_dependencies(ipc_spawn_test quarl-node)
  target_link_libraries(ipc_spawn_test PRIVATE core_interface quarlcoin_node quarlcoin_ipc)
else()
  target_sources(quarlcoin_node PRIVATE ipc/stub.cpp)
endif()

# RPC command-line client library + the quarl-cli executable.
add_library(quarlcoin_cli STATIC EXCLUDE_FROM_ALL
  compat/stdin.cpp
  rpc/client.cpp
)
target_link_libraries(quarlcoin_cli PUBLIC core_interface univalue)

add_executable(quarl-cli quarl-cli.cpp)
add_windows_resources(quarl-cli quarl-cli.rc)
target_link_libraries(quarl-cli core_interface quarlcoin_cli quarlcoin_common quarlcoin_node quarlcoin_util)

# quarl-tx — offline raw-transaction tool.
add_executable(quarl-tx quarl-tx.cpp)
add_windows_resources(quarl-tx quarl-tx.rc)
target_link_libraries(quarl-tx core_interface quarlcoin_common quarlcoin_util univalue)

# quarl-util — offline block-header utility: grind the proof-of-work
# of a hex block header (and netmagic). The Quarlcoin analogue of bitcoin-util.
add_executable(quarl-util quarl-util.cpp)
add_windows_resources(quarl-util quarl-util.rc)
target_link_libraries(quarl-util core_interface quarlcoin_common quarlcoin_consensus quarlcoin_util univalue)

# quarl-miner -- the CPU miner. Two RPCs and a nonce loop; see the note at the
# top of the file for why there is no GPU counterpart in the tree.
add_executable(quarl-miner quarl-miner.cpp)
target_link_libraries(quarl-miner
  core_interface
  quarlcoin_common
  quarlcoin_consensus
  quarlcoin_crypto
  quarlcoin_util
  univalue
)
install_binary_component(quarl-miner)

# Windows: embed the UTF-8 active-code-page manifest on quarl-wallet too.
if(WIN32 AND TARGET quarl-wallet)
  add_windows_resources(quarl-wallet quarl-wallet.rc)
endif()

# Qt GUI (quarl-qt). Opt-in (-DENABLE_GUI=ON); links quarlcoin_node directly.
if(ENABLE_GUI)
  find_package(Qt MODULE REQUIRED)
  add_subdirectory(qt)
endif()

# Test fixtures (always available for benches/fixture tests), the unit tests,
# and the benchmarks each live in their own subdirectory CMakeLists, mirroring
# Bitcoin Core's src/{test/util,test,bench} layout.
add_subdirectory(test/util)
add_subdirectory(test)
add_subdirectory(bench)

# tools/mine_genesis — offline genesis miner (built on demand). Quarlcoin-specific
# (no Core analog), so it stays at the src level.
add_executable(mine_genesis EXCLUDE_FROM_ALL ${CMAKE_SOURCE_DIR}/tools/mine_genesis.cpp)
target_link_libraries(mine_genesis PRIVATE core_interface quarlcoin_node)
