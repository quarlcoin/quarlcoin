# Copyright (c) 2026 The Quarlcoin developers
# See COPYING for license.

add_library(quarlcoin_zmq STATIC EXCLUDE_FROM_ALL
  zmqabstractnotifier.cpp
  zmqnotificationinterface.cpp
  zmqpublishnotifier.cpp
  zmqrpc.cpp
  zmqutil.cpp
)
target_compile_definitions(quarlcoin_zmq
  INTERFACE
    ENABLE_ZMQ=1
)
target_link_libraries(quarlcoin_zmq
  PRIVATE
    core_interface
    univalue
    zeromq
)
