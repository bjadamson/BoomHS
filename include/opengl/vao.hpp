#pragma once
#include <opengl/bind.hpp>
#include <opengl/global.hpp>

#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/glew.hpp>
#include <ostream>
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

  auto gl_raw_value() { return vao_; }
  auto gl_raw_value() const { return vao_; }

  void bind(stlw::Logger&) { global::vao_bind(*this); }
  void unbind(stlw::Logger&) { global::vao_unbind(); }

  std::string to_string() const;
};

std::ostream&
operator<<(std::ostream&, VAO const&);

} // namespace opengl
