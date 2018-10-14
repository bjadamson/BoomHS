#pragma once
#include <boomhs/color.hpp>
#include <opengl/bind.hpp>
#include <opengl/vertex_attribute.hpp>

#include <common/algorithm.hpp>
#include <common/compiler.hpp>
#include <common/optional.hpp>
#include <common/result.hpp>
#include <common/type_macros.hpp>

#include <algorithm>
#include <iostream>

namespace opengl
{

#define DEFINE_SHADER_FILENAME_TYPE(NAME)                                                          \
  struct NAME##ShaderFilename                                                                      \
  {                                                                                                \
    std::string const filename;                                                                    \
    explicit NAME##ShaderFilename(std::string const& fn)                                           \
        : filename(fn)                                                                             \
    {                                                                                              \
    }                                                                                              \
    explicit NAME##ShaderFilename(char const* f)                                                   \
        : filename(f)                                                                              \
    {                                                                                              \
    }                                                                                              \
    MOVE_CONSTRUCTIBLE_ONLY(NAME##ShaderFilename)                                                  \
  }

DEFINE_SHADER_FILENAME_TYPE(Vertex);
DEFINE_SHADER_FILENAME_TYPE(Fragment);
#undef DEFINE_SHADER_FILENAME_TYPE

struct program_factory
{
  program_factory() = delete;

  static Result<GLuint, std::string>
  from_files(common::Logger&, VertexShaderFilename const&, FragmentShaderFilename const&);
};

class ProgramHandle
{
  GLuint program_;

public:
  NO_COPY(ProgramHandle);
  NO_MOVE_ASSIGN(ProgramHandle);

  explicit ProgramHandle(GLuint const);
  ProgramHandle(ProgramHandle&&);
  ~ProgramHandle();

  // public fields
  DebugBoundCheck debug_check;

  auto const& handle() const { return program_; }
};

#ifdef DEBUG_BUILD
struct PathToShaderSources
{
  VertexShaderFilename   vertex;
  FragmentShaderFilename fragment;
};
#endif

class ShaderProgram
{
  ProgramHandle program_;
#ifdef DEBUG_BUILD
  PathToShaderSources source_paths_;
#endif
  VertexAttribute va_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(ShaderProgram);
  explicit ShaderProgram(ProgramHandle&& ph, VertexAttribute&& va
#ifdef DEBUG_BUILD
                         ,
                         PathToShaderSources&& sources
#endif
                         )
      : program_(MOVE(ph))
      , va_(MOVE(va))
#ifdef DEBUG_BUILD
      , source_paths_(MOVE(sources))
#endif
  {
  }

  // public data members
  DebugBoundCheck debug_check;

  std::optional<GLsizei> instance_count = std::nullopt;

  bool is_2d = false;

  // public member fns
  auto const& handle() const { return program_.handle(); }
  auto const& va() const { return this->va_; }

  void bind_impl(common::Logger&);
  void unbind_impl(common::Logger&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();

  GLint get_uniform_location(common::Logger&, GLchar const*);

  void set_uniform_matrix_3fv(common::Logger&, GLchar const*, glm::mat3 const&);
  void set_uniform_matrix_4fv(common::Logger&, GLchar const*, glm::mat4 const&);

  void set_uniform_array_2fv(common::Logger&, GLchar const*, std::array<float, 2> const&);
  void set_uniform_array_3fv(common::Logger&, GLchar const*, std::array<float, 3> const&);
  void set_uniform_array_4fv(common::Logger&, GLchar const*, std::array<float, 4> const&);

  std::string to_string() const;

  void set_uniform_vec2(common::Logger& logger, GLchar const* name, glm::vec2 const& v)
  {
    auto const arr = common::make_array<float>(v.x, v.y);
    set_uniform_array_2fv(logger, name, arr);
  }

  void set_uniform_vec3(common::Logger& logger, GLchar const* name, glm::vec3 const& v)
  {
    auto const arr = common::make_array<float>(v.x, v.y, v.z);
    set_uniform_array_3fv(logger, name, arr);
  }

  void set_uniform_vec3(common::Logger& logger, std::string const& name, glm::vec3 const& v)
  {
    return set_uniform_vec3(logger, name.c_str(), v);
  }

  void set_uniform_vec4(common::Logger& logger, GLchar const* name, glm::vec4 const& v)
  {
    auto const arr = common::make_array<float>(v.x, v.y, v.z, v.w);
    set_uniform_array_4fv(logger, name, arr);
  }

  void set_uniform_vec4(common::Logger& logger, std::string const& name, glm::vec4 const& v)
  {
    return set_uniform_vec4(logger, name.c_str(), v);
  }

  void set_uniform_color(common::Logger& logger, GLchar const* name, boomhs::Color const& c)
  {
    auto const arr = common::make_array<float>(c.r(), c.g(), c.b(), c.a());
    set_uniform_array_4fv(logger, name, arr);
  }

  void set_uniform_color(common::Logger& logger, std::string const& name, boomhs::Color const& c)
  {
    return set_uniform_color(logger, name.c_str(), c);
  }

  void set_uniform_color_3fv(common::Logger& logger, GLchar const* name, boomhs::Color const& c)
  {
    auto const arr = common::make_array<float>(c.r(), c.g(), c.b());
    set_uniform_array_3fv(logger, name, arr);
  }

  void
  set_uniform_color_3fv(common::Logger& logger, std::string const& name, boomhs::Color const& c)
  {
    return set_uniform_color_3fv(logger, name.c_str(), c);
  }

  void set_uniform_float1(common::Logger&, GLchar const*, float const);

  void set_uniform_float1(common::Logger& logger, std::string const& name, float const value)
  {
    return set_uniform_float1(logger, name.c_str(), value);
  }

  void set_uniform_bool(common::Logger&, GLchar const*, bool const);
  void set_uniform_int1(common::Logger&, GLchar const*, int const);
};

class ShaderPrograms
{
  using pair_t = std::pair<std::string, ShaderProgram>;
  std::vector<pair_t> shader_programs_;

public:
  ShaderPrograms() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ShaderPrograms);

  void add(std::string const& s, ShaderProgram&& sp)
  {
    auto pair = std::make_pair(s, MOVE(sp));
    shader_programs_.emplace_back(MOVE(pair));
  }

  std::vector<std::string> all_shader_names() const;

  std::string all_shader_names_flattened(char) const;

  std::optional<size_t> index_of_nickname(std::string const&) const;

  std::optional<std::string> nickname_at_index(size_t) const;

#define LOOKUP_SP(name, begin, end)                                                                \
  auto const lookup_sp = [&](char const* s) {                                                      \
    auto const cmp = [&s](auto const& it) { return it.first == s; };                               \
    auto const it  = std::find_if(begin, end, cmp);                                                \
    assert(end != it);                                                                             \
    return it;                                                                                     \
  }

  ShaderProgram const& ref_sp(char const* s) const
  {
    auto begin = shader_programs_.cbegin();
    auto end   = shader_programs_.cend();
    LOOKUP_SP(s, begin, end);
    return lookup_sp(s)->second;
  }

  ShaderProgram& ref_sp(char const* s)
  {
    auto begin = shader_programs_.begin();
    auto end   = shader_programs_.end();
    LOOKUP_SP(s, begin, end);
    return lookup_sp(s)->second;
  }

#undef LOOKUP_SP
  ShaderProgram const& ref_sp(std::string const& s) const { return ref_sp(s.c_str()); }

  ShaderProgram& ref_sp(std::string const& s) { return ref_sp(s.c_str()); }

  BEGIN_END_FORWARD_FNS(shader_programs_);
};

Result<ShaderProgram, std::string>
make_shader_program(common::Logger&, std::string const&, std::string const&, VertexAttribute&&);

std::ostream&
operator<<(std::ostream&, ShaderProgram const&);

} // namespace opengl
