#pragma once
#include <stlw/type_macros.hpp>

namespace stlw
{
class float_generator;
} // namespace stlw

namespace window
{
class FrameTime;
} // namespace window

namespace boomhs
{
struct RenderState;

class EntityRenderer
{
public:
  EntityRenderer() = default;
  MOVE_CONSTRUCTIBLE_ONLY(EntityRenderer);

  void render(RenderState&, stlw::float_generator&, window::FrameTime const&);
};

class BlackEntityRenderer
{
public:
  BlackEntityRenderer() = default;
  MOVE_CONSTRUCTIBLE_ONLY(BlackEntityRenderer);

  void render(RenderState&, stlw::float_generator&, window::FrameTime const&);
};

} // namespace boomhs
