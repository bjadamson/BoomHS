#pragma once
#include <stlw/sized_buffer.hpp>

namespace opengl
{

struct obj
{
  //stlw::sized_buffer<float> buffer;
  //stlw::sized_buffer<int> mesh_ordering;
  //stlw::sized_buffer<int> indices;
  //stlw::sized_buffer<int> material_indices;
  //unsigned int const num_vertices;
  std::vector<float> buffer;
  std::vector<uint32_t> indices;
};

obj
load_mesh(char const*);

} // ns opengl
