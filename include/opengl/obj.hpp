#pragma once
#include <opengl/glew.hpp>
#include <stlw/sized_buffer.hpp>

namespace opengl
{

struct obj
{
  GLenum const draw_mode;

  std::vector<float> vertices;
  std::vector<uint32_t> indices;
};

obj
load_mesh(char const*, char const*);

} // ns opengl
