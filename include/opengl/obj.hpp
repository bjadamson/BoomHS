#pragma once
#include <stlw/sized_buffer.hpp>

namespace opengl
{

struct obj
{
  std::vector<float> buffer;
  std::vector<uint32_t> indices;
};

obj
load_mesh(char const*);

} // ns opengl
