#pragma once
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

struct ShaderPrograms;

stlw::result<ShaderPrograms, std::string>
load_shader_programs(stlw::Logger &);

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

#define SHADERPROGRAM_DEFAULT_CTOR(CLASSNAME)                                                      \
  MOVE_CONSTRUCTIBLE_ONLY(CLASSNAME);                                                              \
  explicit CLASSNAME(ProgramHandle &&ph, VertexAttribute &&va)                                     \
    : ShaderProgram(MOVE(ph), MOVE(va))                                                            \
    {                                                                                              \
    }

struct ShaderProgramColor2D : public ShaderProgram
{
  SHADERPROGRAM_DEFAULT_CTOR(ShaderProgramColor2D);

  static bool constexpr IS_2D = true;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = false;
};

struct ShaderProgramPositionNormalColor3D : public ShaderProgram
{
  SHADERPROGRAM_DEFAULT_CTOR(ShaderProgramPositionNormalColor3D);

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = true;
};

struct ShaderProgramPositionColor3D : public ShaderProgram
{
  SHADERPROGRAM_DEFAULT_CTOR(ShaderProgramPositionColor3D);

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = false;
};

struct ShaderProgramLightSource3D : public ShaderProgram
{
  SHADERPROGRAM_DEFAULT_CTOR(ShaderProgramLightSource3D);

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  static bool constexpr IS_LIGHTSOURCE = true;
  static bool constexpr RECEIVES_LIGHT = false;
};

struct ShaderProgramAt3D : public ShaderProgram
{
  SHADERPROGRAM_DEFAULT_CTOR(ShaderProgramAt3D);

  auto instance_count() const { return 2u; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = true;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = true;
};

struct ShaderProgramHashtag3D : public ShaderProgram
{
  SHADERPROGRAM_DEFAULT_CTOR(ShaderProgramHashtag3D);

  auto instance_count() const { return 3u; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = true;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = true;
};

struct ShaderProgramPlus3D : public ShaderProgram
{
  SHADERPROGRAM_DEFAULT_CTOR(ShaderProgramPlus3D);

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = true;
};

#define SHADERPROGRAM_TEXTURE_CTOR(CLASSNAME)                                                      \
  explicit CLASSNAME(ProgramHandle &&ph, VertexAttribute &&va, texture_info const t)               \
    : ShaderProgram(MOVE(ph), MOVE(va))                                                            \
    , texture_info_(t)                                                                             \
    {                                                                                              \
    }

#undef SHADERPROGRAM_DEFAULT_CTOR

class ShaderProgramTexture3D : public ShaderProgram
{
  texture_info texture_info_;
public:
  SHADERPROGRAM_TEXTURE_CTOR(ShaderProgramTexture3D);

  auto texture() const { return this->texture_info_; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = false;
};

class ShaderProgramTextureCube3D : public ShaderProgram
{
  texture_info texture_info_;
public:
  SHADERPROGRAM_TEXTURE_CTOR(ShaderProgramTextureCube3D);

  auto texture() const { return this->texture_info_; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = false;
};

class ShaderProgramSkybox3D : public ShaderProgram
{
  texture_info texture_info_;
public:
  SHADERPROGRAM_TEXTURE_CTOR(ShaderProgramSkybox3D);

  auto texture() const { return this->texture_info_; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = true;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = false;
};

class ShaderProgramTexture2D : public ShaderProgram
{
  texture_info texture_info_;
public:
  SHADERPROGRAM_TEXTURE_CTOR(ShaderProgramTexture2D);

  auto texture() const { return this->texture_info_; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = true;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = false;
};
#undef SHADERPROGRAM_TEXTURE_CTOR

struct ShaderPrograms2D
{
  ShaderProgramColor2D color;
  ShaderProgramTexture2D texture_wall;
  ShaderProgramTexture2D texture_container;

  MOVE_CONSTRUCTIBLE_ONLY(ShaderPrograms2D);
};

struct ShaderPrograms3D
{
  ShaderProgramHashtag3D hashtag;
  ShaderProgramAt3D at;
  ShaderProgramPlus3D plus;

  // alphabet
  ShaderProgramPositionNormalColor3D O;
  ShaderProgramPositionNormalColor3D T;

  // 3d arrow (with normals)
  ShaderProgramPositionColor3D local_forward_arrow;

  // 2d arrows
  ShaderProgramPositionColor3D arrow;
  ShaderProgramPositionColor3D color;

  ShaderProgramPositionColor3D global_x_axis_arrow;
  ShaderProgramPositionColor3D global_y_axis_arrow;
  ShaderProgramPositionColor3D global_z_axis_arrow;

  ShaderProgramPositionColor3D local_x_axis_arrow;
  ShaderProgramPositionColor3D local_y_axis_arrow;
  ShaderProgramPositionColor3D local_z_axis_arrow;

  ShaderProgramPositionColor3D camera_arrow0;
  ShaderProgramPositionColor3D camera_arrow1;
  ShaderProgramPositionColor3D camera_arrow2;

  ShaderProgramLightSource3D light0;

  ShaderProgramTextureCube3D texture_cube;
  ShaderProgramTexture3D house;
  ShaderProgramSkybox3D skybox;

  // NORMAL???
  ShaderProgramPositionColor3D terrain;

  MOVE_CONSTRUCTIBLE_ONLY(ShaderPrograms3D);
};

struct ShaderPrograms
{
  ShaderPrograms2D d2;
  ShaderPrograms3D d3;

  MOVE_CONSTRUCTIBLE_ONLY(ShaderPrograms);
  explicit ShaderPrograms(ShaderPrograms2D &&sp2d, ShaderPrograms3D &&sp3d)
    : d2(MOVE(sp2d))
    , d3(MOVE(sp3d))
  {
  }
};

} // ns opengl
