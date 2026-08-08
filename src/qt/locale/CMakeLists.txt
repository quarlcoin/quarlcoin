# Copyright (c) 2026 The Quarlcoin developers
# See COPYING for license.

include(ts_files.cmake)

if(Qt6_VERSION VERSION_GREATER_EQUAL 6.7)
  qt6_add_lrelease(TS_FILES ${ts_files} LRELEASE_TARGET quarlcoinqt_lrelease OPTIONS -silent)
else()
  qt6_add_lrelease(quarlcoinqt TS_FILES ${ts_files} OPTIONS -silent)
endif()

add_dependencies(quarlcoinqt quarlcoinqt_lrelease)
