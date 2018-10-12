#pragma once
#include <common/type_macros.hpp>

namespace boomhs
{
class FrameTime;
class RNG;
} // namespace boomhs

namespace opengl
{
struct RenderState;

class EntityRenderer
{
public:
  EntityRenderer() = default;
  NOCOPY_MOVE_DEFAULT(EntityRenderer);

  void render2d_billboard(RenderState&, boomhs::RNG&, boomhs::FrameTime const&);
  void render2d_ui(RenderState&, boomhs::RNG&, boomhs::FrameTime const&);
  void render3d(RenderState&, boomhs::RNG&, boomhs::FrameTime const&);
};

class SilhouetteEntityRenderer
{
public:
  SilhouetteEntityRenderer() = default;
  NOCOPY_MOVE_DEFAULT(SilhouetteEntityRenderer);

  void render2d_billboard(RenderState&, boomhs::RNG&, boomhs::FrameTime const&);
  void render2d_ui(RenderState&, boomhs::RNG&, boomhs::FrameTime const&);
  void render3d(RenderState&, boomhs::RNG&, boomhs::FrameTime const&);
};

} // namespace opengl
