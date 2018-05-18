#pragma once
#include <boomhs/types.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{
struct TextureInfo;
} // ns opengl

namespace window
{
class FrameTime;
} // ns window

namespace boomhs
{

class Skybox
{
  Transform transform_;
  float speed_;

public:
  Skybox();
  MOVE_CONSTRUCTIBLE_ONLY(Skybox);
  void update(window::FrameTime const&);

  auto& transform() { return transform_; }
};

} // namespace boomhs
