#pragma once
#include <extlibs/glew.hpp>
#include <ostream>
#include <stlw/type_macros.hpp>
#include <string>

namespace opengl
{

class VAO
{
  GLuint vao_ = 0;

  static constexpr auto NUM_BUFFERS = 1;

public:
  NO_COPY(VAO);

  VAO& operator=(VAO&& other)
  {
    vao_       = other.vao_;
    other.vao_ = 0;
    return *this;
  }

  explicit VAO() { glGenVertexArrays(NUM_BUFFERS, &vao_); }

  ~VAO() { glDeleteVertexArrays(NUM_BUFFERS, &vao_); }

  // move-construction OK.
  VAO(VAO&& other)
      : vao_(other.vao_)
  {
    other.vao_ = 0;
  }

  inline auto gl_raw_value() const { return vao_; }

  std::string to_string() const;
};

std::ostream&
operator<<(std::ostream&, VAO const&);

} // namespace opengl
