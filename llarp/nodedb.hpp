#pragma once

#include "crypto/crypto.hpp"
#include "dht/key.hpp"
#include "router_contact.hpp"
#include "router_id.hpp"
#include "util/common.hpp"
#include "util/fs.hpp"
#include "util/thread/threading.hpp"

#include <llarp/router/router.hpp>

#include <algorithm>
#include <atomic>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace llarp
{
  struct Router;

  class NodeDB
  {
    struct Entry
    {
      const RemoteRC rc;
      llarp_time_t insertedAt;
      explicit Entry(RemoteRC rc);
    };

    using NodeMap = std::unordered_map<RouterID, Entry>;

    NodeMap entries;

    const Router& router;
    const fs::path m_Root;
    const std::function<void(std::function<void()>)> disk;

    llarp_time_t m_NextFlushAt;

    /// asynchronously remove the files for a set of rcs on disk given their public ident key
    void
    remove_many_from_disk_async(std::unordered_set<RouterID> idents) const;

    /// get filename of an RC file given its public ident key
    fs::path
    get_path_by_pubkey(RouterID pk) const;

    std::unordered_map<RouterID, RemoteRC> bootstraps;

    // whitelist = active routers
    std::unordered_set<RouterID> router_whitelist;
    // greylist = fully funded, but decommissioned routers
    std::unordered_set<RouterID> router_greylist;
    // greenlist = registered but not fully-staked routers
    std::unordered_set<RouterID> router_greenlist;

    // all registered relays (snodes)
    std::unordered_set<RouterID> registered_routers;

    // only ever use to specific edges as path first-hops
    std::unordered_set<RouterID> pinned_edges;

   public:
    void
    set_bootstrap_routers(const std::set<RemoteRC>& rcs);

    const std::unordered_set<RouterID>&
    whitelist() const
    {
      return router_whitelist;
    }

    const std::unordered_set<RouterID>&
    greylist() const
    {
      return router_greylist;
    }

    const std::unordered_set<RouterID>&
    get_registered_routers() const
    {
      return registered_routers;
    }

    void
    set_router_whitelist(
        const std::vector<RouterID>& whitelist,
        const std::vector<RouterID>& greylist,
        const std::vector<RouterID>& greenlist);

    std::optional<RouterID>
    get_random_whitelist_router() const;

    // client:
    //   if pinned edges were specified, connections are allowed only to those and
    //   to the configured bootstrap nodes.  otherwise, always allow.
    //
    // relay:
    //   outgoing connections are allowed only to other registered, funded relays
    //   (whitelist and greylist, respectively).
    bool
    is_connection_allowed(const RouterID& remote) const;

    // client:
    //   same as is_connection_allowed
    //
    // server:
    //   we only build new paths through registered, not decommissioned relays
    //   (i.e. whitelist)
    bool
    is_path_allowed(const RouterID& remote) const
    {
      return router_whitelist.count(remote);
    }

    // if pinned edges were specified, the remote must be in that set, else any remote
    // is allowed as first hop.
    bool
    is_first_hop_allowed(const RouterID& remote) const;

    const std::unordered_set<RouterID>&
    get_pinned_edges() const
    {
      return pinned_edges;
    }

    explicit NodeDB(
        fs::path rootdir, std::function<void(std::function<void()>)> diskCaller, Router* r);

    /// in memory nodedb
    NodeDB();

    /// load all entries from disk syncrhonously
    void
    load_from_disk();

    /// explicit save all RCs to disk synchronously
    void
    save_to_disk() const;

    /// the number of RCs that are loaded from disk
    size_t
    num_loaded() const;

    /// do periodic tasks like flush to disk and expiration
    void
    Tick(llarp_time_t now);

    /// find the absolute closets router to a dht location
    RemoteRC
    find_closest_to(dht::Key_t location) const;

    /// find many routers closest to dht key
    std::vector<RemoteRC>
    find_many_closest_to(dht::Key_t location, uint32_t numRouters) const;

    /// return true if we have an rc by its ident pubkey
    bool
    has_router(RouterID pk) const;

    /// maybe get an rc by its ident pubkey
    std::optional<RemoteRC>
    get_rc(RouterID pk) const;

    template <typename Filter>
    std::optional<RemoteRC>
    GetRandom(Filter visit) const
    {
      return router.loop()->call_get([visit]() -> std::optional<RemoteRC> {
        std::vector<const decltype(entries)::value_type*> entries;
        for (const auto& entry : entries)
          entries.push_back(entry);

        std::shuffle(entries.begin(), entries.end(), llarp::csrng);

        for (const auto entry : entries)
        {
          if (visit(entry->second.rc))
            return entry->second.rc;
        }

        return std::nullopt;
      });
    }

    /// visit all entries
    template <typename Visit>
    void
    VisitAll(Visit visit) const
    {
      router.loop()->call([this, visit]() {
        for (const auto& item : entries)
          visit(item.second.rc);
      });
    }

    /// visit all entries inserted before a timestamp
    template <typename Visit>
    void
    VisitInsertedBefore(Visit visit, llarp_time_t insertedBefore)
    {
      router.loop()->call([this, visit, insertedBefore]() {
        for (const auto& item : entries)
        {
          if (item.second.insertedAt < insertedBefore)
            visit(item.second.rc);
        }
      });
    }

    /// remove an entry via its ident pubkey
    void
    remove_router(RouterID pk);

    /// remove an entry given a filter that inspects the rc
    template <typename Filter>
    void
    RemoveIf(Filter visit)
    {
      router.loop()->call([this, visit]() {
        std::unordered_set<RouterID> removed;
        auto itr = entries.begin();
        while (itr != entries.end())
        {
          if (visit(itr->second.rc))
          {
            removed.insert(itr->second.rc.router_id());
            itr = entries.erase(itr);
          }
          else
            ++itr;
        }
        if (not removed.empty())
          remove_many_from_disk_async(std::move(removed));
      });
    }

    /// remove rcs that are not in keep and have been inserted before cutoff
    void
    remove_stale_rcs(std::unordered_set<RouterID> keep, llarp_time_t cutoff);

    /// put this rc into the cache if it is not there or newer than the one there already
    void
    put_rc_if_newer(RemoteRC rc);

    /// unconditional put of rc into cache
    void
    put_rc(RemoteRC rc);
  };
}  // namespace llarp
