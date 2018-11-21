#pragma once
#include <extlibs/glm.hpp>

#include <limits>

namespace boomhs::math::constants
{
auto constexpr ZERO = glm::vec3{0};
auto constexpr ONE  = glm::vec3{1};

auto constexpr X_UNIT_VECTOR = glm::vec3{1.0f, 0.0f, 0.0f};
auto constexpr Y_UNIT_VECTOR = glm::vec3{0.0f, 1.0f, 0.0f};
auto constexpr Z_UNIT_VECTOR = glm::vec3{0.0f, 0.0f, 1.0f};

auto constexpr PI       = glm::pi<float>();
auto constexpr TWO_PI   = PI * 2.0f;
auto constexpr EPSILONF = std::numeric_limits<float>::epsilon();

} // namespace boomhs::math::constants
