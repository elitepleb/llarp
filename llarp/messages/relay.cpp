#include "relay.hpp"

#include <llarp/path/path_context.hpp>
#include <llarp/router/abstractrouter.hpp>
#include <llarp/util/bencode.hpp>

namespace llarp
{
  void
  RelayUpstreamMessage::Clear()
  {
    pathid.Zero();
    X.Clear();
    Y.Zero();
    version = 0;
  }

  bool
  RelayUpstreamMessage::BEncode(llarp_buffer_t* buf) const
  {
    if (!bencode_start_dict(buf))
      return false;
    if (!BEncodeWriteDictMsgType(buf, "a", "u"))
      return false;

    if (!BEncodeWriteDictEntry("p", pathid, buf))
      return false;
    if (!BEncodeWriteDictInt("v", llarp::constants::proto_version, buf))
      return false;
    if (!BEncodeWriteDictEntry("x", X, buf))
      return false;
    if (!BEncodeWriteDictEntry("y", Y, buf))
      return false;
    return bencode_end(buf);
  }

  bool
  RelayUpstreamMessage::DecodeKey(const llarp_buffer_t& key, llarp_buffer_t* buf)
  {
    bool read = false;
    if (!BEncodeMaybeReadDictEntry("p", pathid, read, key, buf))
      return false;
    if (!BEncodeMaybeVerifyVersion("v", version, llarp::constants::proto_version, read, key, buf))
      return false;
    if (!BEncodeMaybeReadDictEntry("x", X, read, key, buf))
      return false;
    if (!BEncodeMaybeReadDictEntry("y", Y, read, key, buf))
      return false;
    return read;
  }

  bool
  RelayUpstreamMessage::HandleMessage(AbstractRouter* r) const
  {
    auto path = r->pathContext().GetByDownstream(session->GetPubKey(), pathid);
    if (path)
    {
      path->HandleUpstream(llarp_buffer_t(X), Y, r);
    }
    return true;
  }

  void
  RelayDownstreamMessage::Clear()
  {
    pathid.Zero();
    X.Clear();
    Y.Zero();
    version = 0;
  }

  bool
  RelayDownstreamMessage::BEncode(llarp_buffer_t* buf) const
  {
    if (!bencode_start_dict(buf))
      return false;
    if (!BEncodeWriteDictMsgType(buf, "a", "d"))
      return false;

    if (!BEncodeWriteDictEntry("p", pathid, buf))
      return false;
    if (!BEncodeWriteDictInt("v", llarp::constants::proto_version, buf))
      return false;
    if (!BEncodeWriteDictEntry("x", X, buf))
      return false;
    if (!BEncodeWriteDictEntry("y", Y, buf))
      return false;
    return bencode_end(buf);
  }

  bool
  RelayDownstreamMessage::DecodeKey(const llarp_buffer_t& key, llarp_buffer_t* buf)
  {
    bool read = false;
    if (!BEncodeMaybeReadDictEntry("p", pathid, read, key, buf))
      return false;
    if (!BEncodeMaybeVerifyVersion("v", version, llarp::constants::proto_version, read, key, buf))
      return false;
    if (!BEncodeMaybeReadDictEntry("x", X, read, key, buf))
      return false;
    if (!BEncodeMaybeReadDictEntry("y", Y, read, key, buf))
      return false;
    return read;
  }

  bool
  RelayDownstreamMessage::HandleMessage(AbstractRouter* r) const
  {
    auto path = r->pathContext().GetByUpstream(session->GetPubKey(), pathid);
    if (path)
    {
      path->HandleDownstream(llarp_buffer_t(X), Y, r);
    }
    return true;
  }
}  // namespace llarp
