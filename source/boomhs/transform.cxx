#include <boomhs/transform.hpp>
#include <boomhs/math.hpp>

using namespace boomhs;
using namespace boomhs::math;

namespace
{


template <typename T>
glm::mat4
compute_modelmatrix(T const& transform)
{
  auto const& t = transform.translation;
  auto const& r = transform.rotation;
  auto const& s = transform.scale;
  return math::compute_modelmatrix(t, r, s);
}

} // namespace

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// Transform2D
Transform2D::Transform2D(glm::vec2 const& pos)
    : translation(pos)
    , scale(constants::ONE)
{
}

Transform2D::Transform2D(float const x, float const y)
    : Transform2D(glm::vec2{x, y})
{
}

Transform2D::Transform2D()
    : Transform2D(constants::ZERO)
{
}

glm::mat4
Transform2D::model_matrix() const
{
  return compute_modelmatrix(*this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Transform
Transform::Transform(glm::vec3 const& tr)
    : translation(tr)
    , scale(constants::ONE)
{
}
Transform::Transform(float const x, float const y, float const z)
    : Transform(glm::vec3{x, y, z})
{
}

Transform::Transform()
    : Transform(constants::ZERO)
{
}

glm::mat4
Transform::model_matrix() const
{
  return compute_modelmatrix(*this);
}

} // namespace boomhs
