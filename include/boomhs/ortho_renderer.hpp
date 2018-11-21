#pragma once

namespace opengl
{
struct DrawState;
struct RenderState;
} // namespace opengl

namespace boomhs
{
class  Camera;
class  FrameTime;
class  GameState;
class  LevelManager;
class  RNG;
struct StaticRenderers;

struct OrthoRenderer
{
  OrthoRenderer() = delete;

  static void draw_scene(GameState&, opengl::RenderState&, LevelManager&, opengl::DrawState&, Camera&, RNG& rng,
                         StaticRenderers&, FrameTime const&);
};

} // namespace boomhs
