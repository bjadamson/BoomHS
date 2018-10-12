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
struct GameState;
class  LevelManager;
class  RNG;
class  StaticRenderers;

struct OrthoRenderer
{
  OrthoRenderer() = delete;

  static void draw_scene(GameState&, opengl::RenderState&, LevelManager&, opengl::DrawState&, Camera&, RNG& rng,
                         StaticRenderers&, FrameTime const&);
};

} // namespace boomhs
