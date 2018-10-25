#pragma once
#include <boomhs/color.hpp>
#include <opengl/bind.hpp>
#include <opengl/vertex_attribute.hpp>

#include <common/algorithm.hpp>
#include <common/compiler.hpp>
#include <common/optional.hpp>
#include <common/result.hpp>
#include <common/type_alias.hpp>
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

#define DEFINE_LOOKUP_SP_FN(NAME)                                                                  \
  auto& sp_##NAME() { return ref_sp(#NAME); }                                                      \
  auto const& sp_##NAME() const { return ref_sp(#NAME); }                                          \

  DEFINE_LOOKUP_SP_FN(2dcolor);
  DEFINE_LOOKUP_SP_FN(2dtexture);
  DEFINE_LOOKUP_SP_FN(wireframe);

  DEFINE_LOOKUP_SP_FN(silhoutte_2d);
  DEFINE_LOOKUP_SP_FN(silhoutte_3d);

  DEFINE_LOOKUP_SP_FN(skybox);
  DEFINE_LOOKUP_SP_FN(sunshaft);
  DEFINE_LOOKUP_SP_FN(terrain);

  DEFINE_LOOKUP_SP_FN(water_none);
  DEFINE_LOOKUP_SP_FN(water_basic);
  DEFINE_LOOKUP_SP_FN(water_medium);
  DEFINE_LOOKUP_SP_FN(water_advanced);
#undef DEFINE_LOOKUP_SP_FN

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
template <typename Uniform>
void
set_uniform(common::Logger& logger, ShaderProgram& sp, GLchar const* name, Uniform const& uniform)
{
  static_assert(!std::is_const<Uniform>::value);
  static_assert(!std::is_reference<Uniform>::value);

  using namespace boomhs;
  DEBUG_ASSERT_BOUND(sp);

  // Lookup the location of the uniform in the shader program.
  auto const loc = sp.get_uniform_location(logger, name);

  // Helper function that firsts log's information about the uniform varible being set, and then
  // executes the function to set the uniform variable.
  auto const log_then_set = [&](char const* type_name, auto const& data_string,
                                auto const& set_uniform_fn, auto&&... args) {
    LOG_DEBUG_SPRINTF("Setting OpenGL Program uniform, name: '%s:%s' location: '%d' data: '%s'",
                      name,
                      type_name,
                      loc,
                      data_string);
    set_uniform_fn(FORWARD(args));
  };

  // Setup a bunch of lambda functions that handle capturing the relavent variables so we can make
  // the table below as simple as possible.
  auto const set_pod = [&](auto const& gl_fn, auto const& value, char const* type_name) {
    auto const fn = [&]() { detail::set_uniform_value(loc, gl_fn, value); };
    log_then_set(type_name, std::to_string(value), fn);
  };
  auto const set_array = [&](auto const& gl_fn, auto const& array, char const* type_name) {
    auto const fn = [&]() { detail::set_uniform_array(loc, gl_fn, array.data()); };
    log_then_set(type_name, common::stringify(array), fn);
  };
  auto const set_vecn = [&](auto const& gl_fn, auto const& vecn, char const* type_name) {
    auto const fn = [&]() { detail::set_uniform_array(loc, gl_fn, glm::value_ptr(vecn)); };
    log_then_set(type_name, glm::to_string(vecn), fn);
  };
  auto const set_color = [&](auto const& gl_fn, auto const& color, char const* type_name) {
    if constexpr(TYPES_MATCH(Uniform, ColorRGB)) {
      set_vecn(gl_fn, color.vec3(), type_name);
    }
    else if constexpr(TYPES_MATCH(Uniform, ColorRGBA)) {
      set_vecn(gl_fn, color.vec4(), type_name);
    }
    else {
      LOG_ERROR("Invalid Color Type, cannot set.");
      std::abort();
    }
  };
  auto const set_matrix = [&](auto const& gl_fn, auto const& matrix, char const* type_name) {
    auto const fn = [&]() { detail::set_uniform_matrix(loc, gl_fn, matrix); };
    log_then_set(type_name, glm::to_string(matrix), fn);
  };

  // clang-format on
  // This table uses the above macros to select exactly 1 implementation based on the uniform
  // type. Effectively a bunch of template specialization's, except using "constexpr if" to pick one
  // lambda function to call per-uniform type.
  //
  // The table below maps user types (such as int, vec3, ...) to OpenGL types, for the purposes of
  // setting an opengl program uniform value.
  // Inside the macro **_TYPE_TO_FN (defined below) we call through to the lambdas eventually
  // setting the uniform by calling the OpenGL function provided. This is all handled
  // automatically.
  //
  // Type's not explicitely mapped in the table result in execution ending with a call to
  // std::abort().
  //
  // NOTE: The 'uniform' field must remain in the table below, because it's type is evaluated
  // polymorphically at compile time thanks to "constexpr if" and polymorhic lambdas inside the
  // macro.
  //
  // Unfortunately for now this means 'uniform' must remain as an explicit column. Trying to capture
  // 'uniform' in the helper macros above result's in a compile-time error (this due to the lambda
  // not being in a polymorphic context when instantiated outside of the 'constexpr if').
  //
  // Macro for Mapping a user type -> OpenGL type.
#define IF___MAP_TYPE(UserType, FN, ...)                                                           \
    if constexpr (TYPES_MATCH(Uniform, UserType)) {                                                \
        FN(__VA_ARGS__);                                                                           \
    }
#define ELIF_MAP_TYPE(...) else IF___MAP_TYPE(__VA_ARGS__)

  //              TYPE         HELPER_FN   OPENGL_FN           UNIFORM                  UNIFORM_TYPE
  //              ----------------------------------------------------------------------------------
  IF___MAP_TYPE(int,         set_pod,    glUniform1i,        uniform,                   "int1")
  ELIF_MAP_TYPE(bool,        set_pod,    glUniform1i,        uniform,                   "bool1")
  ELIF_MAP_TYPE(float,       set_pod,    glUniform1f,        uniform,                   "float")

  ELIF_MAP_TYPE(ColorRGB,    set_color,  glUniform3fv,       uniform,                   "ColorRGB")
  ELIF_MAP_TYPE(ColorRGBA,   set_color,  glUniform4fv,       uniform,                   "ColorRGBA")

  ELIF_MAP_TYPE(FloatArray2, set_array,  glUniform2fv,       uniform,                   "array2")
  ELIF_MAP_TYPE(FloatArray3, set_array,  glUniform3fv,       uniform,                   "array3")
  ELIF_MAP_TYPE(FloatArray4, set_array,  glUniform4fv,       uniform,                   "array4")

  ELIF_MAP_TYPE(glm::vec2,   set_vecn,   glUniform2fv,       uniform,                   "vec2")
  ELIF_MAP_TYPE(glm::vec3,   set_vecn,   glUniform3fv,       uniform,                   "vec3")
  ELIF_MAP_TYPE(glm::vec4,   set_vecn,   glUniform4fv,       uniform,                   "vec4")

  ELIF_MAP_TYPE(glm::mat2,   set_matrix, glUniformMatrix2fv, uniform,                   "mat2")
  ELIF_MAP_TYPE(glm::mat3,   set_matrix, glUniformMatrix3fv, uniform,                   "mat3")
  ELIF_MAP_TYPE(glm::mat4,   set_matrix, glUniformMatrix4fv, uniform,                   "mat4")
  else {
    // This code path only gets instantiated by unhandled types.
    LOG_ERROR("Invalid type of uniform, cannot set.");
    std::abort();
  }
  // clang-format off
#undef IF___MAP_TYPE
#undef ELIF_MAP_TYPE
}

template <typename T>
void
set_uniform(common::Logger& logger, ShaderProgram& sp, std::string const& name, T const& value)
{
  set_uniform(logger, sp, name.c_str(), value);
}

} // namespace shader

} // namespace opengl
