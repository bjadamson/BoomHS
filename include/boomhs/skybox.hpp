#pragma once
#include <boomhs/components.hpp>
#include <common/type_macros.hpp>

namespace boomhs
{
class FrameTime;

class Skybox
{
  Transform transform_;
  float     speed_;

public:
  Skybox();
  MOVE_CONSTRUCTIBLE_ONLY(Skybox);
  void update(FrameTime const&);

  auto const& transform() const { return transform_; }
  auto&       transform() { return transform_; }
};

} // namespace boomhs
