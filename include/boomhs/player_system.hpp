#pragma once
#include <boomhs/entity.hpp>
#include <boomhs/state.hpp>
#include <glm/glm.hpp>

namespace s
{
struct player_system
{
  template <typename TData, typename S>
  bool init(TData &tdata, S &state)
  {
    auto const et = ::game::entity_factory::make_transformer(state.logger, tdata);
    ecst::entity_id const eid{boomhs::GameState::AT_INDEX};

    auto const rotate = [&eid, &et](float const angle, auto const& axis) {
      game::entity_transformer<TData>::rotate_entity(et, eid, angle, axis);
    };

    rotate(270.0f, glm::vec3{1.0f, 0.0f, 0.0f});
    rotate(180.0f, glm::vec3{0.0f, 0.0f, 1.0f});
    std::abort();
    return true;
  }

  template <typename TData, typename S>
  void process(TData &tdata, S &state) const
  {
    auto et = ::game::entity_factory::make_transformer(state.logger, tdata);
    ecst::entity_id const eid{boomhs::GameState::AT_INDEX};

    auto const rotate = [&eid, &et](float const angle, auto const& axis) {
      game::entity_transformer<TData>::rotate_entity(et, eid, angle, axis);
    };

    rotate(5.0f, glm::normalize(glm::vec3{1.0f, 1.0f, 1.0f}));
  }
};

} // ns s
