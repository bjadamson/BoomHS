#pragma once
#include <algorithm>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <opengl/colors.hpp>
#include <opengl/vertex_attribute.hpp>
#include <opengl/texture.hpp>
#include <boomhs/types.hpp>

namespace opengl
{

#define DEFINE_SHADER_FILENAME_TYPE(NAME)                                                          \
  struct NAME##_shader_filename {                                                                  \
    char const *filename;                                                                          \
    explicit NAME##_shader_filename(char const *f)                                                 \
        : filename(f)                                                                              \
    {                                                                                              \
    }                                                                                              \
  }

DEFINE_SHADER_FILENAME_TYPE(vertex);
DEFINE_SHADER_FILENAME_TYPE(fragment);
#undef DEFINE_SHADER_FILENAME_TYPE

struct program_factory
{
  program_factory() = delete;

  static stlw::result<GLuint, std::string>
  from_files(vertex_shader_filename const, fragment_shader_filename const);

  static constexpr GLuint
  INVALID_PROGRAM_ID() { return 0; }

  static GLuint
  make_invalid()
  {
    return INVALID_PROGRAM_ID();
  }
};

class ProgramHandle
{
  GLuint program_;

public:
  NO_COPY(ProgramHandle);
  NO_MOVE_ASSIGN(ProgramHandle);

  explicit ProgramHandle(GLuint const);
  ProgramHandle(ProgramHandle &&);
  ~ProgramHandle();

  auto handle() const { return program_; }
};

class ShaderProgram
{
  ProgramHandle program_;
  VertexAttribute va_;
public:
  explicit ShaderProgram(ProgramHandle &&ph, VertexAttribute &&va)
    : program_(MOVE(ph))
    , va_(MOVE(va))
    {
    }

  auto const& va() const { return this->va_; }

  void use_program(stlw::Logger &);
  GLint get_uniform_location(stlw::Logger &, GLchar const *);

  void set_uniform_matrix_4fv(stlw::Logger &, GLchar const *, glm::mat4 const &);

  void set_uniform_array_4fv(stlw::Logger &, GLchar const *, std::array<float, 4> const &);
  void set_uniform_array_3fv(stlw::Logger &, GLchar const*, std::array<float, 3> const&);

  void
  set_uniform_vec3(stlw::Logger &logger, GLchar const* name, glm::vec3 const& v)
  {
    auto const arr = stlw::make_array<float>(v.x, v.y, v.z);
    set_uniform_array_3fv(logger, name, arr);
  }

  void
  set_uniform_color(stlw::Logger &logger, GLchar const* name, Color const& c)
  {
    auto const arr = stlw::make_array<float>(c.r, c.g, c.b, c.a);
    set_uniform_array_4fv(logger, name, arr);
  }

  void
  set_uniform_color_3fv(stlw::Logger &logger, GLchar const* name, Color const& c)
  {
    auto const arr = stlw::make_array<float>(c.r, c.g, c.b);
    set_uniform_array_3fv(logger, name, arr);
  }

  void set_uniform_float1(stlw::Logger &logger, GLchar const*, float const);
};

template<typename ...Args>
static stlw::result<ShaderProgram, std::string>
make_shader_program(char const* vertex_s, char const* fragment_s, VertexAttribute &&va, Args &&... args)
{
  vertex_shader_filename v{vertex_s};
  fragment_shader_filename f{fragment_s};
  DO_TRY(auto sp, program_factory::from_files(v, f));
  return ShaderProgram{MOVE(ProgramHandle{sp}), MOVE(va), std::forward<Args>(args)...};
}

class ShaderPrograms
{
  using pair_t = std::pair<std::string, ShaderProgram>;
  std::vector<pair_t> programs_;
public:
  ShaderPrograms() = default;

  void
  add(std::string const& s, ShaderProgram &&sp)
  {
    auto pair = std::make_pair(s, MOVE(sp));
    programs_.emplace_back(MOVE(pair));
  }

  ShaderProgram&
  sp(std::string const& s)
  {
    auto const cmp = [&s](auto const& it) { return it.first == s; };
    auto const it = std::find_if(programs_.begin(), programs_.end(), cmp);

    assert(programs_.end() != it);
    return it->second;
  }
};

} // ns opengl
