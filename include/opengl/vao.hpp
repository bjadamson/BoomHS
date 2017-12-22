#pragma once
#include <opengl/colors.hpp>
#include <opengl/global.hpp>
#include <boomhs/types.hpp>
#include <opengl/texture.hpp>

namespace opengl
{

class VAO {
  GLuint vao_ = 0;

  static constexpr auto NUM_BUFFERS = 1;

public:
  NO_COPY(VAO);
  NO_MOVE_ASSIGN(VAO);
  explicit VAO()
  {
    glGenVertexArrays(NUM_BUFFERS, &this->vao_);
  }

  ~VAO()
  {
    glDeleteVertexArrays(NUM_BUFFERS, &this->vao_);
  }

  // move-construction OK.
  VAO(VAO &&other)
      : vao_(other.vao_)
  {
    other.vao_ = 0;
  }

  inline auto gl_raw_value() const { return this->vao_; }
};

} // ns opengl
