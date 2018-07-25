#include <boomhs/billboard.hpp>

namespace
{

void
billboard_spherical(float* data)
{
  // Column 0:
  data[0] = 1.0f;
  data[1] = 0.0f;
  data[2] = 0.0f;

  // Column 1:
  data[4 + 0] = 0.0f;
  data[4 + 1] = 1.0f;
  data[4 + 2] = 0.0f;

  // Column 2:
  data[8 + 0] = 0.0f;
  data[8 + 1] = 0.0f;
  data[8 + 2] = 1.0f;
}

void
billboard_cylindrical(float* data)
{
  // Column 0:
  data[0] = 1.0f;
  data[1] = 0.0f;
  data[2] = 0.0f;

  // Column 2:
  data[8 + 0] = 0.0f;
  data[8 + 1] = 0.0f;
  data[8 + 2] = 1.0f;
}

} // namespace

namespace boomhs
{

///////////////////////////////////////////////////////////////////////////////////////////////////
// Billboard
BillboardType
Billboard::from_string(std::string const& str)
{
  if ("spherical" == str) {
    return BillboardType::Spherical;
  }
  else if ("cylindrical" == str) {
    return BillboardType::Cylindrical;
  }
  else {
    std::abort();
  }
}

glm::mat4
Billboard::compute_viewmodel(Transform const& transform, glm::mat4 const& view_matrix,
                             BillboardType const bb_type)
{
  auto view_model = view_matrix * transform.model_matrix();

  // Reset the rotation values in order to achieve a billboard effect.
  //
  // http://www.geeks3d.com/20140807/billboarding-vertex-shader-glsl/
  float* data = glm::value_ptr(view_model);
  switch (bb_type) {
  case BillboardType::Spherical:
    billboard_spherical(data);
    break;
  case BillboardType::Cylindrical:
    billboard_cylindrical(data);
    break;
  default:
    std::abort();
  }

  auto const& s = transform.scale;
  data[0]       = s.x;
  data[5]       = s.y;
  data[10]      = s.z;

  return view_model;
}

} // namespace boomhs
