#include <boomhs/skybox.hpp>
#include <boomhs/state.hpp>

#include <opengl/constants.hpp>
#include <opengl/shader.hpp>
#include <opengl/texture.hpp>
#include <stlw/math.hpp>

#include <cassert>
#include <window/timer.hpp>

using namespace opengl;
using namespace window;

namespace boomhs
{

static constexpr float SKYBOX_SCALE_SIZE = 1000.0f;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Skybox
Skybox::Skybox()
    : speed_(800.0f)
{
  transform_.scale = glm::vec3{SKYBOX_SCALE_SIZE};
}

void
Skybox::update(FrameTime const& ft)
{
  transform_.rotate_degrees(speed_ * ft.delta_millis(), Y_UNIT_VECTOR);
}

} // namespace boomhs
