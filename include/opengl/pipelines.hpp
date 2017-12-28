#pragma once
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <opengl/shader_program.hpp>
#include <opengl/vao.hpp>
#include <opengl/vertex_attribute.hpp>
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
  using info_t = boomhs::color_t;

  static bool constexpr IS_2D = true;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;
};

struct PipelineColor3D : public BasePipeline
{
  PIPELINE_DEFAULT_CTOR(PipelineColor3D);
  using info_t = boomhs::color_t;

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;
};

struct PipelineHashtag3D : public BasePipeline
{
  PIPELINE_DEFAULT_CTOR(PipelineHashtag3D);
  using info_t = boomhs::color_t;

  auto instance_count() const { return 3u; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = true;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;
};

struct PipelineAt3D : public BasePipeline
{
  PIPELINE_DEFAULT_CTOR(PipelineAt3D);
  //using info_t = boomhs::color_t;

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;
};

struct PipelinePlus3D : public BasePipeline
{
  PIPELINE_DEFAULT_CTOR(PipelinePlus3D);
  using info_t = boomhs::color_t;

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;
};

struct PipelineArrow3D : public BasePipeline
{
  PIPELINE_DEFAULT_CTOR(PipelineArrow3D);
  using info_t = boomhs::color_t;

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;
};

struct PipelineAxisArrow3D : public BasePipeline
{
  PIPELINE_DEFAULT_CTOR(PipelineAxisArrow3D);
  using info_t = boomhs::color_t;

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;
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
  using info_t = boomhs::uv_t;

  auto texture() const { return this->texture_info_; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;
};

class PipelineTextureCube3D : public BasePipeline
{
  texture_info texture_info_;
public:
  PIPELINE_TEXTURE_CTOR(PipelineTextureCube3D);
  using info_t = boomhs::uv_t;

  auto texture() const { return this->texture_info_; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;
};

class PipelineSkybox3D : public BasePipeline
{
  texture_info texture_info_;
public:
  PIPELINE_TEXTURE_CTOR(PipelineSkybox3D);
  using info_t = boomhs::uv_t;

  auto texture() const { return this->texture_info_; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = true;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;
};

class PipelineTexture2D : public BasePipeline
{
  texture_info texture_info_;
public:
  PIPELINE_TEXTURE_CTOR(PipelineTexture2D);
  using info_t = boomhs::uv_t;

  auto texture() const { return this->texture_info_; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = true;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;
};
#undef PIPELINE_TEXTURE_CTOR

template<bool IS_2D_T>
class PipelineWireframe : public BasePipeline
{
  Color color_;
public:
  explicit PipelineWireframe(ShaderProgram &&sp, VertexAttribute &&va, Color const& color)
    : BasePipeline(MOVE(sp), MOVE(va))
    , color_(color)
    {
    }
  using info_t = boomhs::wireframe_t;

  auto color() const { return this->color_; }

  static bool constexpr IS_2D = IS_2D_T;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = true;
  static bool constexpr HAS_TEXTURE = false;
};

using PipelineWireframe2D = PipelineWireframe<true>;
using PipelineWireframe3D = PipelineWireframe<false>;

struct Pipeline2D
{
  PipelineColor2D color;
  PipelineTexture2D texture_wall;
  PipelineTexture2D texture_container;
  PipelineWireframe2D wireframe;

  MOVE_CONSTRUCTIBLE_ONLY(Pipeline2D);
};

struct Pipeline3D
{
  PipelineColor3D color;
  PipelineHashtag3D hashtag;
  PipelineAt3D at;
  PipelinePlus3D plus;
  PipelineArrow3D arrow;

  PipelineAxisArrow3D global_x_axis_arrow;
  PipelineAxisArrow3D global_y_axis_arrow;
  PipelineAxisArrow3D global_z_axis_arrow;

  PipelineAxisArrow3D local_x_axis_arrow;
  PipelineAxisArrow3D local_y_axis_arrow;
  PipelineAxisArrow3D local_z_axis_arrow;

  PipelineAxisArrow3D local_forward_arrow;

  PipelineAxisArrow3D camera_arrow0;
  PipelineAxisArrow3D camera_arrow1;
  PipelineAxisArrow3D camera_arrow2;

  PipelineTextureCube3D texture_cube;
  PipelineTexture3D house;
  PipelineSkybox3D skybox;
  PipelineColor3D terrain;
  PipelineWireframe3D wireframe;

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
