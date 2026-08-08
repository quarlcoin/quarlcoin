// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_TEST_UTIL_JSON_H
#define QUARLCOIN_TEST_UTIL_JSON_H

#include <univalue.h>

#include <string_view>

UniValue read_json(std::string_view jsondata);

#endif // QUARLCOIN_TEST_UTIL_JSON_H
