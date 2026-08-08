// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_COMMON_LICENSE_INFO_H
#define QUARLCOIN_COMMON_LICENSE_INFO_H

#include <string>

std::string CopyrightHolders(const std::string& strPrefix);

/** Returns licensing information (for -version) */
std::string LicenseInfo();

#endif // QUARLCOIN_COMMON_LICENSE_INFO_H
