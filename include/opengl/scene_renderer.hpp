#pragma once
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/glm.hpp>

namespace boomhs
{
class LevelManager;
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
class RenderState;

class DefaultSceneRenderer
{
public:
  MOVE_CONSTRUCTIBLE_ONLY(DefaultSceneRenderer);
  DefaultSceneRenderer() = default;

  void render_scene(RenderState&, boomhs::LevelManager&, stlw::float_generator&,
                    window::FrameTime const&);
};

class BlackSceneRenderer
{
public:
  MOVE_CONSTRUCTIBLE_ONLY(BlackSceneRenderer);
  BlackSceneRenderer() = default;

  void render_scene(RenderState&, boomhs::LevelManager&, stlw::float_generator&,
                    window::FrameTime const&);
};

} // namespace opengl
