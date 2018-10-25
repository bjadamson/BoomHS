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

  std::string to_string() const;

  GLint get_uniform_location(common::Logger&, GLchar const*);
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

namespace detail
{
template <typename F, typename T>
void
set_uniform_value(GLint const loc, F const& f, T const& value)
{
  f(loc, value);
}

template <typename F, typename A>
void
set_uniform_array(GLint const loc, F const& f, A const& array)
{

  // https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
  //
  // For the vector (glUniform*v) commands, specifies the number of elements that are to be
  // modified.
  // This should be 1 if the targeted uniform variable is not an array, and 1 or more if it is an
  // array.
  GLsizei constexpr COUNT = 1;
  f(loc, COUNT, array);
}

template <typename F, typename T>
void
set_uniform_matrix(GLint const loc, F const& f, T const& matrix)
{
  // https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
  //
  // count:
  // For the matrix (glUniformMatrix*) commands, specifies the number of matrices that are to be
  // modified.
  // This should be 1 if the targeted uniform variable is not an array of matrices, and 1 or more
  // if it is an array of matrices.
  GLsizei constexpr COUNT                = 1;
  GLboolean constexpr TRANSPOSE_MATRICES = GL_FALSE;

  f(loc, COUNT, TRANSPOSE_MATRICES, glm::value_ptr(matrix));
}

} // namespace detail

namespace shader
{

// Set a uniform variable for the shader program (argument).
//
// This function maps user types to types understood by OpenGL, ie: primitives, array's, and
// matrices.
//
// This mapping from user-type to OpenGL type happens at compile time.
template <typename U, size_t N>
void
set_uniform(common::Logger& logger, ShaderProgram& sp, GLchar const* name, U const& uniform)
{
  DEBUG_ASSERT_BOUND(sp);
  auto const loc         = sp.get_uniform_location(logger, name);
  auto const log_uniform = [&](char const* type_name, auto const& data_string) {
    LOG_DEBUG_SPRINTF("Setting OpenGL Program uniform, name: '%s:%s' location: '%d' data: '%s'",
                      name,
                      type_name,
                      loc,
                      data_string);
  };
  auto const set_then_log = [&loc](auto const& f, auto const& value, char const* type_name,
                               auto const& data_string) {
    f(loc, value, type_name);
    log_uniform(type_name, data_string);
  };

  auto const set_value = [&](auto const& f, auto const& value, char const* type_name) {
    set_then_log(&detail::set_uniform_value, value, type_name, std::to_string(value));
  };

  auto const set_array = [&](auto const& f, std::array<U, N> const& array, char const* type_name) {
    set_then_log(&detail::set_uniform_array, array, type_name, common::stringify(array));
  };
  auto const set_vecn = [&](auto const& f, auto const& vecn, char const* type_name) {
    set_array(f, glm::value_ptr(vecn), type_name);
  };
  auto const set_matrix = [&](auto const& f, auto const& matrix, char const* type_name) {
    set_then_log(&detail::set_uniform_matrix, matrix, type_name, glm::to_string(matrix));
  };

  using UniformType = typename std::remove_reference<typename std::remove_const<U>::type>::type;
#define TYPE_TO_FN(TYPE, FN) if constexpr (std::is_same<UniformType, TYPE>::value) { [&]() { FN; }(); }

using Array2 = std::array<float, 2>;
using Array3 = std::array<float, 3>;
using Array4 = std::array<float, 4>;
using namespace boomhs;

// clang-format on
       TYPE_TO_FN(int,       set_value(glUniform1i,         uniform,                 "int1"))
  else TYPE_TO_FN(bool,      set_value(glUniform1i,         uniform,                 "bool1"))
  else TYPE_TO_FN(float,     set_value(glUniform1f,         uniform,                 "float"))

  else TYPE_TO_FN(ColorRGB,  set_vecn(glUniform3fv,         uniform.vec3(),          "ColorRGB"))
  else TYPE_TO_FN(ColorRGBA, set_vecn(glUniform4fv,         uniform.vec4(),          "ColorRGBA"))

  else TYPE_TO_FN(Array2,    set_array(glUniform2fv,        uniform.data(),          "array2"))
  else TYPE_TO_FN(Array3,    set_array(glUniform3fv,        uniform.data(),          "array3"))
  else TYPE_TO_FN(Array4,    set_array(glUniform4fv,        uniform.data(),          "array4"))

  else TYPE_TO_FN(glm::vec2, set_array(glUniform2fv,        glm::value_ptr(uniform), "vec2"))
  else TYPE_TO_FN(glm::vec3, set_array(glUniform3fv,        glm::value_ptr(uniform), "vec3"))
  else TYPE_TO_FN(glm::vec4, set_array(glUniform4fv,        glm::value_ptr(uniform), "vec4"))

  else TYPE_TO_FN(glm::mat2, set_matrix(glUniformMatrix2fv, uniform,                 "mat2"))
  else TYPE_TO_FN(glm::mat3, set_matrix(glUniformMatrix3fv, uniform,                 "mat3"))
  else TYPE_TO_FN(glm::mat4, set_matrix(glUniformMatrix4fv, uniform,                 "mat4"))
  else {
    // This code path only gets instantiated by unhandled types.
    LOG_ERROR("Invalid type of uniform, cannot set.");
    std::abort();
  }
// clang-format off
#undef MAP_TYPE_TO_FN
}

template <typename T>
void
set_uniform(common::Logger& logger, ShaderProgram& sp, std::string const& name, T const& value)
{
  set_uniform(logger, sp, name.c_str(), value);
}

} // namespace shader

} // namespace opengl
