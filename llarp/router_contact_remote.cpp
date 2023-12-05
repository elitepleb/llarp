#include "constants/version.hpp"
#include "crypto/crypto.hpp"
#include "net/net.hpp"
#include "router_contact.hpp"
#include "util/bencode.hpp"
#include "util/buffer.hpp"
#include "util/file.hpp"
#include "util/time.hpp"

#include <oxenc/bt_serialize.h>

namespace llarp
{
  RemoteRC::RemoteRC(oxenc::bt_dict_consumer btdc)
  {
    try
    {
      bt_load(btdc);

      btdc.require_signature("~", [this](ustring_view msg, ustring_view sig) {
        if (sig.size() != 64)
          throw std::runtime_error{"Invalid signature: not 64 bytes"};

        if (is_expired(time_now_ms()))
          throw std::runtime_error{"Unable to verify expired RemoteRC!"};

        // TODO: revisit if this is needed; detail from previous implementation
        const auto* net = net::Platform::Default_ptr();

        if (net->IsBogon(addr().in4()) and BLOCK_BOGONS)
        {
          auto err = "Unable to verify expired RemoteRC!";
          log::info(logcat, err);
          throw std::runtime_error{err};
        }

        if (not crypto::verify(router_id(), msg, sig))
          throw std::runtime_error{"Failed to verify RemoteRC"};
      });
    }
    catch (const std::exception& e)
    {
      log::warning(logcat, "Failed to parse RemoteRC: {}", e.what());
      throw;
    }
  }

  void
  RemoteRC::bt_verify(oxenc::bt_dict_consumer& data, bool reject_expired) const
  {
    data.require_signature("~", [this, reject_expired](ustring_view msg, ustring_view sig) {
      if (sig.size() != 64)
        throw std::runtime_error{"Invalid signature: not 64 bytes"};

      if (reject_expired and is_expired(time_now_ms()))
        throw std::runtime_error{"Rejecting expired RemoteRC!"};

      // TODO: revisit if this is needed; detail from previous implementation
      const auto* net = net::Platform::Default_ptr();

      if (net->IsBogon(addr().in4()) and BLOCK_BOGONS)
      {
        auto err = "Unable to verify expired RemoteRC!";
        log::info(logcat, err);
        throw std::runtime_error{err};
      }

      if (not crypto::verify(router_id(), msg, sig))
        throw std::runtime_error{"Failed to verify RemoteRC"};
    });
  }

  bool
  RemoteRC::read(const fs::path& fname)
  {
    ustring buf;
    buf.reserve(MAX_RC_SIZE);

    try
    {
      util::file_to_buffer(fname, buf.data(), MAX_RC_SIZE);

      oxenc::bt_dict_consumer btdc{buf};
      bt_load(btdc);
      bt_verify(btdc);

      _payload = buf;
    }
    catch (const std::exception& e)
    {
      log::error(logcat, "Failed to read or validate RC from {}: {}", fname, e.what());
      return false;
    }

    return true;
  }

  bool
  RemoteRC::verify() const
  {
    oxenc::bt_dict_consumer btdc{_payload};
    bt_verify(btdc);
    return true;
  }

}  // namespace llarp