# Copyright (c) 2026 The Quarlcoin developers
# See COPYING for license.

# BLAKE3, from crypto/blake3/ -- see the UPSTREAM note there.
#
# A library of its own, because it has two consumers that must share one copy:
# quarlcoin_crypto below, and the secp256k1 subtree, which hashes with BLAKE3
# since its own SHA-256 was replaced. Compiling it into both would put two
# copies of every symbol in front of the linker.
#
# The portable kernel is always compiled; the SIMD ones are added under the same
# probes this file already uses for the SHA-256 kernels. BLAKE3_NO_AVX512
# because the tree probes for SSE4.1 and AVX2 and has no probe for AVX-512;
# BLAKE3_USE_NEON is settled explicitly rather than left to the library's own
# autodetection, so that what a build compiles is decided in one place.
add_library(quarlcoin_blake3 STATIC
  blake3/blake3.c
  blake3/blake3_dispatch.c
  blake3/blake3_portable.c
)
target_include_directories(quarlcoin_blake3 PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_compile_definitions(quarlcoin_blake3 PUBLIC BLAKE3_NO_AVX512)
set_target_properties(quarlcoin_blake3 PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
  target_sources(quarlcoin_blake3 PRIVATE blake3/blake3_neon.c)
  target_compile_definitions(quarlcoin_blake3 PUBLIC BLAKE3_USE_NEON=1)
else()
  target_compile_definitions(quarlcoin_blake3 PUBLIC BLAKE3_USE_NEON=0)
endif()

if(HAVE_SSE41)
  target_sources(quarlcoin_blake3 PRIVATE blake3/blake3_sse2.c blake3/blake3_sse41.c)
  set_property(SOURCE blake3/blake3_sse41.c PROPERTY COMPILE_OPTIONS ${SSE41_CXXFLAGS})
else()
  target_compile_definitions(quarlcoin_blake3 PUBLIC BLAKE3_NO_SSE2 BLAKE3_NO_SSE41)
endif()

if(HAVE_AVX2)
  target_sources(quarlcoin_blake3 PRIVATE blake3/blake3_avx2.c)
  set_property(SOURCE blake3/blake3_avx2.c PROPERTY COMPILE_OPTIONS ${AVX2_CXXFLAGS})
else()
  target_compile_definitions(quarlcoin_blake3 PUBLIC BLAKE3_NO_AVX2)
endif()

add_library(quarlcoin_crypto STATIC EXCLUDE_FROM_ALL
  aes.cpp
  chacha20.cpp
  chacha20poly1305.cpp
  hex_base.cpp
  hkdf_sha256_32.cpp
  hmac_sha256.cpp
  hmac_sha512.cpp
  muhash.cpp
  poly1305.cpp
  ripemd160.cpp
  sha1.cpp
  sha256.cpp
  sha256_sse4.cpp
  sha3.cpp
  sha512.cpp
  siphash.cpp
  ../support/cleanse.cpp
)

target_link_libraries(quarlcoin_crypto
  PRIVATE
    core_interface
  PUBLIC
    quarlcoin_blake3
)

if(HAVE_SSE41)
  target_compile_definitions(quarlcoin_crypto PRIVATE ENABLE_SSE41)
  target_sources(quarlcoin_crypto PRIVATE sha256_sse41.cpp)
  set_property(SOURCE sha256_sse41.cpp PROPERTY
    COMPILE_OPTIONS ${SSE41_CXXFLAGS}
  )
endif()

if(HAVE_AVX2)
  target_compile_definitions(quarlcoin_crypto PRIVATE ENABLE_AVX2)
  target_sources(quarlcoin_crypto PRIVATE sha256_avx2.cpp)
  set_property(SOURCE sha256_avx2.cpp PROPERTY
    COMPILE_OPTIONS ${AVX2_CXXFLAGS}
  )
endif()

if(HAVE_SSE41 AND HAVE_X86_SHANI)
  target_compile_definitions(quarlcoin_crypto PRIVATE ENABLE_SSE41 ENABLE_X86_SHANI)
  target_sources(quarlcoin_crypto PRIVATE sha256_x86_shani.cpp)
  set_property(SOURCE sha256_x86_shani.cpp PROPERTY
    COMPILE_OPTIONS ${SSE41_CXXFLAGS} ${X86_SHANI_CXXFLAGS}
  )
endif()

if(HAVE_ARM_SHANI)
  target_compile_definitions(quarlcoin_crypto PRIVATE ENABLE_ARM_SHANI)
  target_sources(quarlcoin_crypto PRIVATE sha256_arm_shani.cpp)
  set_property(SOURCE sha256_arm_shani.cpp PROPERTY
    COMPILE_OPTIONS ${ARM_SHANI_CXXFLAGS}
  )
endif()
