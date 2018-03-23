#pragma once
#include <opengl/colors.hpp>
#include <opengl/draw_info.hpp>

#include <array>
#include <vector>

namespace boomhs
{
struct Obj;
class TileGrid;
} // namespace boomhs

namespace opengl
{
class ShaderProgram;
} // namespace opengl

namespace opengl::factories
{

struct RectInfo
{
  static constexpr auto NUM_VERTICES = 4;
  float                 width, height;

  // use one, not both (checked in debug builds)
  std::optional<Color>                           color;
  std::optional<std::array<Color, NUM_VERTICES>> colors;

  std::optional<std::array<glm::vec2, NUM_VERTICES>> uvs;
};

struct RectBuffer
{
  std::vector<float>    vertices;
  std::array<GLuint, 6> indices;
};

RectBuffer
create_rectangle(RectInfo const&);

} // namespace opengl::factories

namespace OF = opengl::factories;
