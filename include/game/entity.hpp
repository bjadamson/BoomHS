#pragma once
#include <engine/gfx/colors.hpp>
#include <game/data_types.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/format.hpp>

namespace game
{

template<typename L, typename TData>
class entity_transformer
{
  // TODO: Make no copy/ no move once we can use template argument deduction for constructors,
  // and we get get rid of the make_entity_transformer() fn.
  //NO_COPY(entity_transformer);
  //NO_MOVE(entity_transformer);

  // private
  L &logger;
  TData &data;

  friend struct entity_factory;
  entity_transformer(L &l, TData &d)
    : logger(l)
    , data(d)
  {}

  template<typename FN>
  void
  for_entity(ecst::entity_id const eid, char const* action, FN const& fn)
  {
    this->logger.trace(fmt::sprintf("'%s' entity eid '%d'", action, eid));
    fn();
  }

  void
  move_entity(ecst::entity_id const eid, glm::vec3 const& distance)
  {
    auto const fn = [&]() {
      auto &m = this->data.get(ct::model, eid);
      m.translation += distance;
    };
    for_entity(eid, "moving", fn);
  }

  void rotate_entity(ecst::entity_id const eid, float const angle, glm::vec3 const& axis)
  {
    auto const fn = [&]() {
      auto &m = this->data.get(ct::model, eid);
      auto const new_rotation = glm::angleAxis(glm::radians(angle), axis);
      m.rotation = new_rotation * m.rotation;
    };
    for_entity(eid, "rotating", fn);
  }

  void scale_entity(ecst::entity_id const eid, glm::vec3 const& sf)
  {
    auto const fn = [&]() {
      auto &m = this->data.get(ct::model, eid);
      m.scale += sf;
    };
    for_entity(eid, "scaling", fn);
  }
public:
  void move_entities(glm::vec3 const& direction)
  {
    data.for_entities([&](auto const eid) {
        move_entity(eid, direction);
    });
  }

  void rotate_entities(float const angle, glm::vec3 const& axis)
  {
    data.for_entities([&](auto const eid) {
        rotate_entity(eid, angle, axis);
    });
  }

  void scale_entities(glm::vec3 const& sf)
  {
    data.for_entities([&](auto const eid) {
        scale_entity(eid, sf);
    });
  }
};

struct entity_factory
{
  entity_factory() = delete;

  template<typename L, typename TData>
  static auto make_transformer(L &logger, TData &data)
  {
    return entity_transformer<L, TData>(logger, data);
  }
};

} // ns game
