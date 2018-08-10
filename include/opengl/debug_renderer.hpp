#pragma once
#include <common/log.hpp>
#include <common/type_macros.hpp>

#include <extlibs/glm.hpp>

namespace boomhs
{
class LevelManager;
class RNG;
} // namespace boomhs

namespace window
{
class FrameTime;
} // namespace window

namespace opengl
{
class RenderState;

class DebugRenderer
{
public:
  MOVE_CONSTRUCTIBLE_ONLY(DebugRenderer);
  DebugRenderer() = default;

  void render_scene(RenderState&, boomhs::LevelManager&, boomhs::RNG&, window::FrameTime const&);
};

class BlackSceneRenderer
{
public:
  MOVE_CONSTRUCTIBLE_ONLY(BlackSceneRenderer);
  BlackSceneRenderer() = default;

  void render_scene(RenderState&, boomhs::LevelManager&, boomhs::RNG&, window::FrameTime const&);
};

} // namespace opengl
