// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_UTIL_BATCHPRIORITY_H
#define QUARLCOIN_UTIL_BATCHPRIORITY_H

/**
 * On platforms that support it, tell the kernel the calling thread is
 * CPU-intensive and non-interactive. See SCHED_BATCH in sched(7) for details.
 *
 */
void ScheduleBatchPriority();

#endif // QUARLCOIN_UTIL_BATCHPRIORITY_H