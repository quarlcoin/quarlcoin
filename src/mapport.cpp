// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#include <mapport.h>

// Quarlcoin ships no NAT-PMP/PCP/UPnP port mapping, so these are no-ops.
void StartMapPort(bool) {}
void InterruptMapPort() {}
void StopMapPort() {}
