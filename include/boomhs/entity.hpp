#pragma once
#include <opengl/colors.hpp>
#include <boomhs/ecst.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/format.hpp>
#include <stlw/log.hpp>

// THIS IS ALL REUSABLE CODE, MOVE IT TO ENGINE folder.
namespace game
{

template<typename PROXY>
class entity_transformer
{
  // TODO: Make no copy/ no move once we can use template argument deduction for constructors,
  // and we get get rid of the make_entity_transformer() fn.
  NO_COPY(entity_transformer);

  stlw::Logger &logger;
  PROXY &proxy;

  friend struct entity_factory;
  entity_transformer(stlw::Logger &l, PROXY &d)
    : logger(l)
    , proxy(d)
  {}

  template<typename FN>
  void
  for_entity(ecst::entity_id const eid, char const* action, FN const& fn) const
  {
    LOG_TRACE(fmt::sprintf("'%s' entity eid '%d'", action, eid));
    fn();
  }
public:
  MOVE_DEFAULT(entity_transformer);
  void move_entities(glm::vec3 const& direction) const
  {
    proxy.for_entities([&](auto const eid) {
        entity_transformer::move_entity(*this, eid, direction);
    });
  }

  void rotate_entities(float const angle, glm::vec3 const& axis) const
  {
    proxy.for_entities([&](auto const eid) {
        entity_transformer::rotate_entity(*this, eid, angle, axis);
    });
  }

  void scale_entities(float const factor) const
  {
    proxy.for_entities([&](auto const eid) {
        entity_transformer::scale_entity(*this, eid, factor);
    });
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // static fns
  template<typename E>
  void static rotate_entity(E const& transformer, ecst::entity_id const eid, float const angle,
      glm::vec3 const& axis)
  {
    auto const fn = [&]() {
      auto &m = transformer.proxy.get(ct::model, eid);
      auto const new_rotation = glm::angleAxis(glm::radians(angle), axis);
      m.rotation = new_rotation * m.rotation;
    };
    transformer.for_entity(eid, "rotating", fn);
  }

  template<typename E>
  void static move_entity(E const& transformer, ecst::entity_id const eid, glm::vec3 const& distance)
  {
    auto const fn = [&]() {
      auto &m = transformer.proxy.get(ct::model, eid);
      m.translation += distance;
    };
    transformer.for_entity(eid, "moving", fn);
  }

  template<typename E>
  void static scale_entity(E const& transformer, ecst::entity_id const eid, float const factor)
  {
    auto const fn = [&]() {
      auto &m = transformer.proxy.get(ct::model, eid);
      m.scale *= factor;
    };
    transformer.for_entity(eid, "scaling", fn);
  }
};

struct entity_factory
{
  entity_factory() = delete;

  template<typename PROXY>
  static auto make_transformer(stlw::Logger &logger, PROXY &proxy)
  {
    return entity_transformer<PROXY>(logger, proxy);
  }
};

} // ns game
