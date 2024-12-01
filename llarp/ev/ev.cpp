#include "ev.hpp"
#include <llarp/util/mem.hpp>
#include <llarp/util/str.hpp>

#include <cstddef>
#include <cstring>
#include <string_view>

#include "libuv.hpp"
#include <llarp/net/net.hpp>

namespace llarp
{
  EventLoop_ptr
  EventLoop::create(size_t queueLength)
  {
    return std::make_shared<llarp::uv::Loop>(queueLength);
  }

  const net::Platform*
  EventLoop::Net_ptr() const
  {
    return net::Platform::Default_ptr();
  }

  EventLoopWork::EventLoopWork(std::function<void(bool)> cleanup) : _cleanup{std::move(cleanup)}
  {}

  void
  EventLoopWork::work() const
  {
    for (const auto& work : _pure_work)
      work();
  }

  void
  EventLoopWork::cleanup(bool cancel) const
  {
    if (_cleanup)
      _cleanup(cancel);
  }

}  // namespace llarp
