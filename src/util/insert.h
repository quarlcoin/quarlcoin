// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.
#ifndef QUARLCOIN_UTIL_INSERT_H
#define QUARLCOIN_UTIL_INSERT_H

#include <set>

namespace util {

//! Simplification of std insertion
template <typename Tdst, typename Tsrc>
inline void insert(Tdst& dst, const Tsrc& src) {
    dst.insert(dst.begin(), src.begin(), src.end());
}
template <typename TsetT, typename Tsrc>
inline void insert(std::set<TsetT>& dst, const Tsrc& src) {
    dst.insert(src.begin(), src.end());
}

template <typename TsetT, typename Compare, typename Tsrc>
inline void insert(std::set<TsetT, Compare>& dst, const Tsrc& src) {
    dst.insert(src.begin(), src.end());
}

} // namespace util

#endif // QUARLCOIN_UTIL_INSERT_H
