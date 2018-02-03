#pragma once
#include <opengl/colors.hpp>
#include <opengl/vertex_attribute.hpp>
#include <boomhs/types.hpp>

#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <algorithm>
#include <boost/optional.hpp>

namespace opengl
{

#define DEFINE_SHADER_FILENAME_TYPE(NAME)                                                          \
  struct NAME##_shader_filename {                                                                  \
    std::string const filename;                                                                    \
    explicit NAME##_shader_filename(std::string const& fn)                                         \
        : filename(fn)                                                                             \
    {                                                                                              \
    }                                                                                              \
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

  // public data members
  boost::optional<GLsizei> instance_count = boost::none;

  bool is_skybox = false;
  bool is_2d = false;

  // public member fns
  auto handle() const { return program_.handle(); }
  auto const& va() const { return this->va_; }

  void use_program(stlw::Logger &);
  GLint get_uniform_location(stlw::Logger &, GLchar const *);

  void set_uniform_matrix_3fv(stlw::Logger &, GLchar const *, glm::mat3 const &);
  void set_uniform_matrix_4fv(stlw::Logger &, GLchar const *, glm::mat4 const &);

  void set_uniform_array_3fv(stlw::Logger &, GLchar const*, std::array<float, 3> const&);
  void set_uniform_array_4fv(stlw::Logger &, GLchar const *, std::array<float, 4> const &);

  void
  set_uniform_vec3(stlw::Logger &logger, GLchar const* name, glm::vec3 const& v)
  {
    auto const arr = stlw::make_array<float>(v.x, v.y, v.z);
    set_uniform_array_3fv(logger, name, arr);
  }

  void
  set_uniform_vec3(stlw::Logger &logger, std::string const& name, glm::vec3 const& v)
  {
    return set_uniform_vec3(logger, name.c_str(), v);
  }

  void
  set_uniform_color(stlw::Logger &logger, GLchar const* name, Color const& c)
  {
    auto const arr = stlw::make_array<float>(c.r(), c.g(), c.b(), c.a());
    set_uniform_array_4fv(logger, name, arr);
  }

  void
  set_uniform_color(stlw::Logger &logger, std::string const& name, Color const& c)
  {
    return set_uniform_color(logger, name.c_str(), c);
  }

  void
  set_uniform_color_3fv(stlw::Logger &logger, GLchar const* name, Color const& c)
  {
    auto const arr = stlw::make_array<float>(c.r(), c.g(), c.b());
    set_uniform_array_3fv(logger, name, arr);
  }

  void
  set_uniform_color_3fv(stlw::Logger &logger, std::string const& name, Color const& c)
  {
    return set_uniform_color_3fv(logger, name.c_str(), c);
  }

  void set_uniform_float1(stlw::Logger &, GLchar const*, float const);

  void set_uniform_float1(stlw::Logger &logger, std::string const& name, float const value)
  {
    return set_uniform_float1(logger, name.c_str(), value);
  }

  void set_uniform_bool(stlw::Logger &, GLchar const*, bool const);
  void set_uniform_int1(stlw::Logger &, GLchar const*, int const);
};

std::ostream&
operator<<(std::ostream&, ShaderProgram const&);

inline stlw::result<ShaderProgram, std::string>
make_shader_program(std::string const& vertex_s, std::string const& fragment_s, VertexAttribute &&va)
{
  vertex_shader_filename v{vertex_s};
  fragment_shader_filename f{fragment_s};
  DO_TRY(auto sp, program_factory::from_files(v, f));
  return ShaderProgram{ProgramHandle{sp}, MOVE(va)};
}

class ShaderPrograms
{
  using pair_t = std::pair<std::string, ShaderProgram>;
  std::vector<pair_t> shader_programs_;

public:
  ShaderPrograms() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ShaderPrograms);

  void
  add(std::string const& s, ShaderProgram &&sp)
  {
    auto pair = std::make_pair(s, MOVE(sp));
    shader_programs_.emplace_back(MOVE(pair));
  }

#define LOOKUP_SP(name)                                                                   \
  auto const lookup_sp = [this](char const* s) {                                          \
      auto const cmp = [&s, this](auto const& it) { return it.first == s; };              \
      auto const it = std::find_if(shader_programs_.begin(), shader_programs_.end(), cmp);\
      assert(shader_programs_.end() != it);                                               \
      return it;                                                                          \
  }

  ShaderProgram const&
  ref_sp(char const* s) const
  {
    LOOKUP_SP(s);
    return lookup_sp(s)->second;
  }

  ShaderProgram&
  ref_sp(char const* s)
  {
    LOOKUP_SP(s);
    return lookup_sp(s)->second;
  }

#undef LOOKUP_SP
  ShaderProgram const&
  ref_sp(std::string const& s) const
  {
    return ref_sp(s.c_str());
  }

  ShaderProgram&
  ref_sp(std::string const& s)
  {
    return ref_sp(s.c_str());
  }

  BEGIN_END_FORWARD_FNS(shader_programs_);
};

} // ns opengl
