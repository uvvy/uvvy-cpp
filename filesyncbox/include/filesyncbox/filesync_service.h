#pragma once

#include <boost/filesystem.hpp>
#include <boost/signals2/signal.hpp>
#include "sss/host.h"

namespace filesyncbox {

/**
 * File sync service monitors given directories and keeps these files synchronised between
 * given instances. There are several sync policies applicable for different level of trust
 * between peer nodes.
 */
class filesync_service
{
    class private_impl;
    std::shared_ptr<private_impl> pimpl_;

public:
    filesync_service(std::shared_ptr<sss::host> host);
    ~filesync_service();
    bool is_active() const;

    void add_directory_sync(boost::filesystem::path dir, std::vector<sss::peer_identity> const& to_peers);

    // File sync service signals
    using completion_signal = boost::signals2::signal<void (void)>;
    completion_signal sync_completed;
};

} // filesyncbox namespace

