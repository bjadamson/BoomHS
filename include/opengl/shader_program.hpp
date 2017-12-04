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
    NAME##_shader_filename(char const *f)                                                          \
        : filename(f)                                                                              \
    {                                                                                              \
    }                                                                                              \
  }

namespace opengl
{

DEFINE_SHADER_FILENAME_TYPE(vertex);
DEFINE_SHADER_FILENAME_TYPE(fragment);

#undef DEFINE_SHADER_FILENAME_TYPE

struct program_factory {
  program_factory() = delete;

  static stlw::result<GLuint, std::string>
  from_files(vertex_shader_filename const, fragment_shader_filename const);

  static GLuint make_invalid();
};

// Essentially a "handle" over the program-object (GLuint) native OpenGL provides, but adds C++
// move-semantics.
class ShaderProgram
{
  GLuint program_;
  explicit ShaderProgram(GLuint const p)
      : program_(p)
  {
  }

  static void
  destroy(ShaderProgram &);

  friend struct ShaderProgramFactory;
  NO_COPY(ShaderProgram);
  NO_MOVE_ASSIGN(ShaderProgram);
public:
  ShaderProgram(ShaderProgram &&);
  ~ShaderProgram();

  void check_errors(stlw::Logger &);
  void use();

  void set_uniform_matrix_4fv(stlw::Logger &, GLchar const*, glm::mat4 const &);
  void set_uniform_array_4fv(stlw::Logger &, GLchar const*, std::array<float, 4> const &);
  void set_uniform_array_3fv(stlw::Logger &, GLchar const*, glm::vec3 const&);
};

struct ShaderProgramFactory
{
  ShaderProgramFactory() = delete;

  static stlw::result<ShaderProgram, std::string>
  make(vertex_shader_filename const v, fragment_shader_filename const f)
  {
    DO_TRY(auto program, program_factory::from_files(v, f));
    return ShaderProgram{program};
  }
};

} // ns opengl
