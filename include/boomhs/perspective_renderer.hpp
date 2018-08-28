#pragma once

namespace opengl
{
struct DrawState;
class RenderState;
} // namespace opengl

namespace boomhs
{
class  Camera;
class  FrameTime;
class  LevelManager;
class  RNG;
class  StaticRenderers;

struct PerspectiveRenderer
{
  PerspectiveRenderer() = delete;

  static void
  draw_scene(opengl::RenderState&, LevelManager&, opengl::DrawState&, Camera&, RNG& rng,
             StaticRenderers&, FrameTime const&);
};

} // namespace boomhs
