#pragma once
#include <opengl/glew.hpp>
#include <stlw/sized_buffer.hpp>

namespace opengl
{

struct obj
{
  GLenum const draw_mode;

  unsigned int num_vertices;
  std::vector<float> vertices;
  std::vector<uint32_t> indices;
};

struct LoadNormals
{
  bool const value = false;
};

struct LoadUvs
{
  bool const value = false;
};

obj
load_mesh(char const*, char const*, LoadNormals const, LoadUvs const);

obj
load_mesh(char const*, LoadNormals const, LoadUvs const);

} // ns opengl
