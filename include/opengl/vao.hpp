#pragma once
#include <extlibs/glew.hpp>
#include <opengl/global.hpp>
#include <stlw/type_macros.hpp>

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

  inline auto gl_raw_value() const { return vao_; }

  std::string to_string() const;

  template <typename FN>
  void while_bound(FN const& fn) const
  {
    global::vao_bind(*this);
    ON_SCOPE_EXIT([&]() { global::vao_unbind(); });
    fn();
  }
};

std::ostream&
operator<<(std::ostream&, VAO const&);

} // namespace opengl
