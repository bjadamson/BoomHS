#pragma once
#include <game/boomhs/ecst_components.hpp>

namespace s
{

struct randompos_system {

  template<typename L>
  bool init(L &logger)
  {
    logger.trace("randompos::init()");
    return true;
  }

  template <typename TData, typename S>
  void process(TData &data, S &state)
  {
    state.logger.trace("randompos::process(data, state)");
    data.for_entities([&](auto eid)
        {
            auto &v = data.get(ct::world_coordinate, eid);
            //v.set(30.0f, 30.0f, 30.0f, 30.0f);/
        });
  }
};

} // ns s
