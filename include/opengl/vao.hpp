#pragma once
#include <opengl/glew.hpp>
#include <stlw/type_macros.hpp>
#include <string>
#include <ostream>

namespace opengl
{

class VAO
{
  GLuint vao_ = 0;

  static constexpr auto NUM_BUFFERS = 1;
public:
  NO_COPY(VAO);

  VAO& operator=(VAO &&other)
  {
    vao_ = other.vao_;
    other.vao_ = 0;
    return *this;
  }

  explicit VAO()
  {
    glGenVertexArrays(NUM_BUFFERS, &vao_);
  }

  ~VAO()
  {
    glDeleteVertexArrays(NUM_BUFFERS, &vao_);
  }

  // move-construction OK.
  VAO(VAO &&other)
      : vao_(other.vao_)
  {
    other.vao_ = 0;
  }

  inline auto gl_raw_value() const { return vao_; }

  friend std::ostream& operator<<(std::ostream &, VAO const&);
};

inline std::ostream&
operator<<(std::ostream &stream, VAO const& vao)
{
  stream << "NUM_BUFFERS: '";
  stream << VAO::NUM_BUFFERS;
  stream << "' vao_: '";
  stream << std::to_string(vao.gl_raw_value()) << "'";
  return stream;
}

} // ns opengl
