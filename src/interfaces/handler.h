// Copyright (c) 2026 The Quarlcoin developers
// See COPYING for license.

#ifndef QUARLCOIN_INTERFACES_HANDLER_H
#define QUARLCOIN_INTERFACES_HANDLER_H

#include <util/btcsignals.h>

#include <functional>
#include <memory>


namespace interfaces {

//! Generic interface for managing an event handler or callback function
//! registered with another interface. Has a single disconnect method to cancel
//! the registration and prevent any future notifications.
class Handler
{
public:
    virtual ~Handler() = default;

    //! Disconnect the handler.
    virtual void disconnect() = 0;
};

//! Return handler wrapping a boost signal connection.
std::unique_ptr<Handler> MakeSignalHandler(btcsignals::connection connection);

//! Return handler wrapping a cleanup function.
std::unique_ptr<Handler> MakeCleanupHandler(std::function<void()> cleanup);

} // namespace interfaces

#endif // QUARLCOIN_INTERFACES_HANDLER_H
