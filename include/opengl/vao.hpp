#pragma once
#include <opengl/colors.hpp>
#include <opengl/global.hpp>
#include <opengl/shader_program.hpp>
#include <opengl/types.hpp>
#include <opengl/texture.hpp>

#include <backward/backward.hpp>

namespace opengl
{

class VAO {
  GLuint vao_ = 0;

  static constexpr auto NUM_BUFFERS = 1;

public:
  explicit VAO()
  {
    glGenVertexArrays(NUM_BUFFERS, &this->vao_);
  }

  NO_COPY(VAO);
  NO_MOVE_ASSIGN(VAO);

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
