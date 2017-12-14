#pragma once
#include <boomhs/ecst_components.hpp>

namespace s
{

struct randompos_system {

  template <typename TData, typename S>
  bool init(TData &tdata, S &state)
  {
    auto &logger = state.logger;
    LOG_TRACE("randompos::init()");
    return true;
  }

  template <typename TData, typename S>
  void process(TData &data, S &state)
  {
    state.LOG_TRACE("randompos::process(data, state)");
    data.for_entities([&](auto const eid) {
      state.LOG_TRACE(fmt::sprintf("eid '%d'", eid));
    });
  }
};

} // ns s
