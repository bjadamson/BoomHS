#pragma once
#include <common/log.hpp>
#include <common/type_macros.hpp>

#include <extlibs/glm.hpp>

namespace boomhs
{
class Camera;
class FrameTime;
class LevelManager;
class RNG;
} // namespace boomhs

namespace opengl
{
class RenderState;

class DebugRenderer
{
public:
  MOVE_CONSTRUCTIBLE_ONLY(DebugRenderer);
  DebugRenderer() = default;

  void render_scene(RenderState&, boomhs::LevelManager&, boomhs::Camera&, boomhs::RNG&,
                    boomhs::FrameTime const&);
};

class BlackSceneRenderer
{
public:
  MOVE_CONSTRUCTIBLE_ONLY(BlackSceneRenderer);
  BlackSceneRenderer() = default;

  void render_scene(RenderState&, boomhs::LevelManager&, boomhs::Camera&, boomhs::RNG&,
                    boomhs::FrameTime const&);
};

} // namespace opengl
