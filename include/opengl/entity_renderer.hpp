#pragma once
#include <stlw/type_macros.hpp>

namespace boomhs
{
struct RenderState;
} // namespace boomhs

namespace stlw
{
class float_generator;
} // namespace stlw

namespace window
{
class FrameTime;
} // namespace window

namespace opengl
{

class EntityRenderer
{
public:
  EntityRenderer() = default;
  MOVE_CONSTRUCTIBLE_ONLY(EntityRenderer);

  void render(boomhs::RenderState&, stlw::float_generator&, window::FrameTime const&);
};

class BlackEntityRenderer
{
public:
  BlackEntityRenderer() = default;
  MOVE_CONSTRUCTIBLE_ONLY(BlackEntityRenderer);

  void render(boomhs::RenderState&, stlw::float_generator&, window::FrameTime const&);
};

} // namespace opengl
