#pragma once
#include <common/type_macros.hpp>

namespace boomhs
{
class RNG;
} // namespace boomhs

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

  void render2d_billboard(RenderState&, boomhs::RNG&, window::FrameTime const&);
  void render2d_ui(RenderState&, boomhs::RNG&, window::FrameTime const&);
  void render3d(RenderState&, boomhs::RNG&, window::FrameTime const&);
};

class SilhouetteEntityRenderer
{
public:
  SilhouetteEntityRenderer() = default;
  MOVE_CONSTRUCTIBLE_ONLY(SilhouetteEntityRenderer);

  void render2d_billboard(RenderState&, boomhs::RNG&, window::FrameTime const&);
  void render2d_ui(RenderState&, boomhs::RNG&, window::FrameTime const&);
  void render3d(RenderState&, boomhs::RNG&, window::FrameTime const&);
};

} // namespace opengl
