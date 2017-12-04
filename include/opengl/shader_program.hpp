#pragma once
#include <opengl/gl_log.hpp>
#include <opengl/glew.hpp>
#include <opengl/global.hpp>
#include <opengl/vertex_attribute.hpp>

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stlw/format.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#define DEFINE_SHADER_FILENAME_TYPE(NAME)                                                          \
  struct NAME##_shader_filename {                                                                  \
    char const *filename;                                                                          \
    explicit NAME##_shader_filename(char const *f)                                                 \
        : filename(f)                                                                              \
    {                                                                                              \
    }                                                                                              \
  }

namespace opengl
{

DEFINE_SHADER_FILENAME_TYPE(vertex);
DEFINE_SHADER_FILENAME_TYPE(fragment);

#undef DEFINE_SHADER_FILENAME_TYPE

// Essentially a "handle" over the program-object (GLuint) native OpenGL provides, but adds C++
// move-semantics.
class ShaderProgram
{
  GLuint program_;

public:
  NO_COPY(ShaderProgram);
  NO_MOVE_ASSIGN(ShaderProgram);

  explicit ShaderProgram(GLuint const p)
      : program_(p)
  {
  }

  ShaderProgram(ShaderProgram &&);
  ~ShaderProgram();

  void use(stlw::Logger &);

  GLint
  get_uniform_location(stlw::Logger &, GLchar const *);
};

stlw::result<ShaderProgram, std::string>
make_shader_program(vertex_shader_filename const, fragment_shader_filename const);

} // ns opengl
