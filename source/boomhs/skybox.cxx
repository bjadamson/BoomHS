#include <boomhs/skybox.hpp>
#include <boomhs/state.hpp>

#include <opengl/shader.hpp>
#include <opengl/texture.hpp>
#include <boomhs/math.hpp>

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
    : speed_(10.0f)
{
  transform_.scale = glm::vec3{SKYBOX_SCALE_SIZE};
}

void
Skybox::update(FrameTime const& ft)
{
  transform_.rotate_degrees(speed_ * ft.delta_millis(), math::EulerAxis::Y);
}

} // namespace boomhs
