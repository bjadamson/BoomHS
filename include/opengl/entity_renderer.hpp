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

namespace opengl
{
struct RenderState;

class EntityRenderer
{
public:
  EntityRenderer() = default;
  NO_MOVE(EntityRenderer);
  NO_COPY(EntityRenderer);

  void render2d_billboard(RenderState&, stlw::float_generator&, window::FrameTime const&);
  void render2d_ui(RenderState&, stlw::float_generator&, window::FrameTime const&);
  void render3d(RenderState&, stlw::float_generator&, window::FrameTime const&);
};

class BlackEntityRenderer
{
public:
  BlackEntityRenderer() = default;
  MOVE_CONSTRUCTIBLE_ONLY(BlackEntityRenderer);

  void render2d_billboard(RenderState&, stlw::float_generator&, window::FrameTime const&);
  void render2d_ui(RenderState&, stlw::float_generator&, window::FrameTime const&);
  void render3d(RenderState&, stlw::float_generator&, window::FrameTime const&);
};

} // namespace opengl
