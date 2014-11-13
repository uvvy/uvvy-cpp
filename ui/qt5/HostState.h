#pragma once

#include <thread>
#include "sss/host.h"
#include "arsenal/settings_provider.h"
#include "traverse_nat.h"

/**
 * HostState embeds settings, NAT traversal and io_service runner for the SSS host.
 */
class HostState
{
    std::shared_ptr<settings_provider> settings_;
    std::shared_ptr<sss::host> host_;
    std::shared_ptr<upnp::UpnpIgdClient> nat_;
    std::thread runner_; // Thread to run io_service (@todo Could be a thread pool)

public:
    HostState()
        : settings_(settings_provider::instance())
        , host_(sss::host::create(settings_))
        , runner_([this] { host_->run_io_service(); })
    {
        nat_ = traverse_nat(host_);
    }

    ~HostState()
    {
        host_->get_io_service().stop();
        runner_.join();
    }

    inline std::shared_ptr<sss::host> host() const { return host_; }
    inline std::shared_ptr<settings_provider> settings() const { return settings_; }
};

