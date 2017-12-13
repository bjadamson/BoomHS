#pragma once
#include <boomhs/state.hpp>

namespace s
{
struct player_system
{
  template <typename TData, typename S>
  bool init(TData &tdata, S &state)
  {
    auto et = ::game::entity_factory::make_transformer(state.logger, tdata);

    ecst::entity_id const eid{boomhs::GameState::AT_INDEX};
    glm::vec3 const axis{1.0f, 0.0f, 0.0f};
    game::entity_transformer<TData>::rotate_entity(et, eid, 90.0f, axis);
    return true;
  }

  template <typename TData, typename S>
  void process(TData &tdata, S &state, SDL_Event &event) const
  {
  }
};

} // ns s
