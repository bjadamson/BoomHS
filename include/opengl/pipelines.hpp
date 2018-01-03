#pragma once
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <opengl/colors.hpp>
#include <opengl/shader_program.hpp>
#include <opengl/vao.hpp>
#include <opengl/vertex_attribute.hpp>
#include <opengl/texture.hpp>
#include <boomhs/types.hpp>

namespace opengl
{

struct OpenglPipelines;

stlw::result<OpenglPipelines, std::string>
load_pipelines(stlw::Logger &);

class BasePipeline
{
  ShaderProgram program_;
  VertexAttribute va_;
  VAO vao_;
public:
  explicit BasePipeline(ShaderProgram &&sp, VertexAttribute &&va)
    : program_(MOVE(sp))
    , va_(MOVE(va))
    {
    }

  auto const& va() const { return this->va_; }
  auto const& vao() const { return this->vao_; }

  void set_uniform_matrix_4fv(stlw::Logger &, GLchar const *, glm::mat4 const &);

  void set_uniform_array_4fv(stlw::Logger &, GLchar const *, std::array<float, 4> const &);
  void set_uniform_array_3fv(stlw::Logger &, GLchar const*, std::array<float, 3> const&);

  void
  set_uniform_array_vec3(stlw::Logger &logger, GLchar const* name, glm::vec3 const& v)
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

  void use_program(stlw::Logger &);
};

#define PIPELINE_DEFAULT_CTOR(CLASSNAME)                                                           \
  MOVE_CONSTRUCTIBLE_ONLY(CLASSNAME);                                                              \
  explicit CLASSNAME(ShaderProgram &&sp, VertexAttribute &&va)                                     \
    : BasePipeline(MOVE(sp), MOVE(va))                                                             \
    {                                                                                              \
    }

struct PipelineColor2D : public BasePipeline
{
  PIPELINE_DEFAULT_CTOR(PipelineColor2D);

  static bool constexpr IS_2D = true;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = false;
};

struct PipelinePositionNormalColor3D : public BasePipeline
{
  PIPELINE_DEFAULT_CTOR(PipelinePositionNormalColor3D);

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = true;
};

struct PipelinePositionColor3D : public BasePipeline
{
  PIPELINE_DEFAULT_CTOR(PipelinePositionColor3D);

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = false;
};

struct PipelineLightSource3D : public BasePipeline
{
  PIPELINE_DEFAULT_CTOR(PipelineLightSource3D);

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  static bool constexpr IS_LIGHTSOURCE = true;
  static bool constexpr RECEIVES_LIGHT = false;
};

struct PipelineHashtag3D : public BasePipeline
{
  PIPELINE_DEFAULT_CTOR(PipelineHashtag3D);

  auto instance_count() const { return 3u; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = true;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = true;
};

struct PipelinePlus3D : public BasePipeline
{
  PIPELINE_DEFAULT_CTOR(PipelinePlus3D);

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = true;
};

#define PIPELINE_TEXTURE_CTOR(CLASSNAME)                                                           \
  explicit CLASSNAME(ShaderProgram &&sp, VertexAttribute &&va, texture_info const t)               \
    : BasePipeline(MOVE(sp), MOVE(va))                                                             \
    , texture_info_(t)                                                                             \
    {                                                                                              \
    }

#undef PIPELINE_DEFAULT_CTOR

class PipelineTexture3D : public BasePipeline
{
  texture_info texture_info_;
public:
  PIPELINE_TEXTURE_CTOR(PipelineTexture3D);

  auto texture() const { return this->texture_info_; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = false;
};

class PipelineTextureCube3D : public BasePipeline
{
  texture_info texture_info_;
public:
  PIPELINE_TEXTURE_CTOR(PipelineTextureCube3D);

  auto texture() const { return this->texture_info_; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = false;
};

class PipelineSkybox3D : public BasePipeline
{
  texture_info texture_info_;
public:
  PIPELINE_TEXTURE_CTOR(PipelineSkybox3D);

  auto texture() const { return this->texture_info_; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = true;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = false;
};

class PipelineTexture2D : public BasePipeline
{
  texture_info texture_info_;
public:
  PIPELINE_TEXTURE_CTOR(PipelineTexture2D);

  auto texture() const { return this->texture_info_; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = true;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  static bool constexpr IS_LIGHTSOURCE = false;
  static bool constexpr RECEIVES_LIGHT = false;
};
#undef PIPELINE_TEXTURE_CTOR

struct Pipeline2D
{
  PipelineColor2D color;
  PipelineTexture2D texture_wall;
  PipelineTexture2D texture_container;

  MOVE_CONSTRUCTIBLE_ONLY(Pipeline2D);
};

struct Pipeline3D
{
  PipelineHashtag3D hashtag;
  PipelinePositionNormalColor3D at;
  PipelinePlus3D plus;

  // alphabet
  PipelinePositionNormalColor3D O;
  PipelinePositionNormalColor3D T;

  // 3d arrow (with normals)
  PipelinePositionColor3D local_forward_arrow;

  // 2d arrows
  PipelinePositionColor3D arrow;
  PipelinePositionColor3D color;

  PipelinePositionColor3D global_x_axis_arrow;
  PipelinePositionColor3D global_y_axis_arrow;
  PipelinePositionColor3D global_z_axis_arrow;

  PipelinePositionColor3D local_x_axis_arrow;
  PipelinePositionColor3D local_y_axis_arrow;
  PipelinePositionColor3D local_z_axis_arrow;

  PipelinePositionColor3D camera_arrow0;
  PipelinePositionColor3D camera_arrow1;
  PipelinePositionColor3D camera_arrow2;

  PipelineLightSource3D light0;

  PipelineTextureCube3D texture_cube;
  PipelineTexture3D house;
  PipelineSkybox3D skybox;

  // NORMAL???
  PipelinePositionColor3D terrain;

  MOVE_CONSTRUCTIBLE_ONLY(Pipeline3D);
};

struct OpenglPipelines
{
  Pipeline2D d2;
  Pipeline3D d3;

  MOVE_CONSTRUCTIBLE_ONLY(OpenglPipelines);
  explicit OpenglPipelines(Pipeline2D &&p2d, Pipeline3D &&p3d)
    : d2(MOVE(p2d))
    , d3(MOVE(p3d))
  {
  }
};

} // ns opengl
