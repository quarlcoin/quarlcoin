# Copyright (c) 2026 The Quarlcoin developers
# See COPYING for license.
#
# Finds the ZeroMQ library (the optional pub/sub notification interface) and
# exposes it as the imported `zeromq` target.
#
# The CMake package is tried first and pkg-config only after it. That order is
# not a preference, it is a correctness requirement for cross builds: depends
# installs a ZeroMQ CMake config for the target it built, but no libzmq.pc, so a
# pkg-config-only search falls through to the *host* pkg-config, finds the Linux
# libzmq, and puts /usr/include on the command line of a mingw compile. What
# comes out is not a missing library -- it is glibc's stdlib.h colliding with
# mingw's corecrt.h, a hundred errors deep in the standard headers, with nothing
# in the message to say that a search order caused it.

if(TARGET zeromq)
  return()
endif()

# CONFIG mode honours CMAKE_FIND_ROOT_PATH, so a cross build looks only where
# the toolchain says its target libraries are.
find_package(ZeroMQ ${ZeroMQ_FIND_VERSION} QUIET CONFIG)

if(ZeroMQ_FOUND AND TARGET libzmq-static)
  add_library(zeromq INTERFACE IMPORTED)
  target_link_libraries(zeromq INTERFACE libzmq-static)
elseif(ZeroMQ_FOUND AND TARGET libzmq)
  add_library(zeromq INTERFACE IMPORTED)
  target_link_libraries(zeromq INTERFACE libzmq)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(libzmq REQUIRED IMPORTED_TARGET libzmq)
  add_library(zeromq INTERFACE IMPORTED)
  target_link_libraries(zeromq INTERFACE PkgConfig::libzmq)
  set(ZeroMQ_VERSION ${libzmq_VERSION})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZeroMQ
  REQUIRED_VARS ZeroMQ_VERSION
  VERSION_VAR ZeroMQ_VERSION)
