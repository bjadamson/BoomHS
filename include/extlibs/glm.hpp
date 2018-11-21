#pragma once
#define GLM_FORCE_CTOR_INIT
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_PURE
#define GLM_EXT_INCLUDED
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/vector_query.hpp>

#include <ostream>

#define VEC2 glm::vec2
#define VEC3 glm::vec3
#define VEC4 glm::vec4

#define IVEC2 glm::ivec2
#define IVEC3 glm::ivec3
#define IVEC4 glm::ivec4

namespace glm
{

inline std::ostream&
operator<<(std::ostream& stream, glm::vec2 const& vec)
{
  stream << to_string(vec);
  return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, glm::vec3 const& vec)
{
  stream << to_string(vec);
  return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, glm::vec4 const& vec)
{
  stream << to_string(vec);
  return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, glm::mat2 const& mat)
{
  stream << to_string(mat);
  return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, glm::mat3 const& mat)
{
  stream << to_string(mat);
  return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, glm::mat4 const& mat)
{
  stream << to_string(mat);
  return stream;
}

inline std::ostream&
operator<<(std::ostream& stream, glm::quat const& quat)
{
  stream << to_string(quat);
  return stream;
}

} // namespace glm
