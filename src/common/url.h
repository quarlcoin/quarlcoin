// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_COMMON_URL_H
#define QUARLCOIN_COMMON_URL_H

#include <string>
#include <string_view>

/* Decode a URL.
 *
 * Notably this implementation does not decode a '+' to a ' '.
 */
std::string UrlDecode(std::string_view url_encoded);

/* Encode a URL. */
std::string UrlEncode(std::string_view str);

#endif // QUARLCOIN_COMMON_URL_H
