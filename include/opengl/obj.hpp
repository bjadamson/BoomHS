#pragma once
#include <opengl/colors.hpp>
#include <opengl/glew.hpp>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_macros.hpp>

namespace opengl
{

struct LoadNormals
{
  bool const value = false;
};

struct LoadUvs
{
  bool const value = false;
};

struct obj
{
  GLenum const draw_mode;

  unsigned int num_vertices;
  std::vector<float> vertices;
  std::vector<uint32_t> indices;

  MOVE_CONSTRUCTIBLE_ONLY(obj);
};

class ObjLoader
{
  Color color_;
public:
  explicit ObjLoader(Color const& c)
    : color_(c)
  {
  }

  obj
  load_mesh(char const*, char const*, LoadNormals const, LoadUvs const) const;

  obj
  load_mesh(char const*, LoadNormals const, LoadUvs const) const;

  void set_color(Color const& c)
  {
    color_ = c;
  }
};

} // ns opengl
