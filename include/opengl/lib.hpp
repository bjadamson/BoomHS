#pragma once
#include <stlw/burrito.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <opengl/camera.hpp>
#include <opengl/shader_program.hpp>
#include <opengl/vao.hpp>

namespace opengl
{

class BasePipeline
{
  ShaderProgram program_;
  vertex_attribute va_;
  VAO vao_;
public:
  explicit BasePipeline(ShaderProgram &&sp, vertex_attribute &&va)
    : program_(MOVE(sp))
    , va_(MOVE(va))
    {
    }

  auto const& va() const { return this->va_; }
  auto const& vao() const { return this->vao_; }
  auto& program_ref() { return this->program_; }
};

#define PIPELINE_CTOR(CLASSNAME)                                                                   \
  MOVE_CONSTRUCTIBLE_ONLY(CLASSNAME);                                                              \
  explicit CLASSNAME(ShaderProgram &&sp, vertex_attribute &&va)                                    \
    : BasePipeline(MOVE(sp), MOVE(va))                                                             \
    {                                                                                              \
    }

struct PipelineColor2D : public BasePipeline
{
  PIPELINE_CTOR(PipelineColor2D);
  using info_t = color_t;

  static bool constexpr IS_2D = true;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;
};

struct PipelineColor3D : public BasePipeline
{
  PIPELINE_CTOR(PipelineColor3D);
  using info_t = color_t;

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;
};

struct PipelineHashtag3D : public BasePipeline
{
  PIPELINE_CTOR(PipelineHashtag3D);
  using info_t = color_t;

  auto instance_count() const { return 3u; }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = true;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;
};

#define PIPELINE_TEXTURE_CTOR(CLASSNAME)                                                           \
  explicit CLASSNAME(ShaderProgram &&sp, vertex_attribute &&va, texture_info const t)              \
    : BasePipeline(MOVE(sp), MOVE(va))                                                             \
    , texture_info_(t)                                                                             \
    {                                                                                              \
    }

class PipelineTexture3D : public BasePipeline
{
  texture_info texture_info_;
public:
  PIPELINE_TEXTURE_CTOR(PipelineTexture3D);
  using info_t = uv_t;

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
  using info_t = uv_t;

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
  using info_t = uv_t;

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
  using info_t = uv_t;

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
  std::array<float, 4> color_;
public:
  explicit PipelineWireframe(ShaderProgram &&sp, vertex_attribute &&va, std::array<float, 4> const& c)
    : BasePipeline(MOVE(sp), MOVE(va))
    , color_(c)
    {
    }
  using info_t = wireframe_t;

  auto color() const { return this->color_; }

  static bool constexpr IS_2D = IS_2D_T;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = true;
  static bool constexpr HAS_TEXTURE = false;
};

using PipelineWireframe2D = PipelineWireframe<true>;
using PipelineWireframe3D = PipelineWireframe<false>;

#undef PIPELINE_CTOR

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
  PipelineTextureCube3D texture_cube;
  PipelineTexture3D house;
  PipelineSkybox3D skybox;
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

namespace detail
{
  template<typename R, typename ...Args>
  static stlw::result<R, std::string>
  make_pipeline(char const* vertex_s, char const* frag_s, vertex_attribute &&va, Args &&... args)
  {
    DO_TRY(auto sp, ShaderProgramFactory::make(vertex_s, frag_s));
    return R{MOVE(sp), MOVE(va), std::forward<Args>(args)...};
  }
} // ns detail

stlw::result<OpenglPipelines, std::string>
load_resources(stlw::Logger &logger)
{
  using detail::make_pipeline;

  DO_TRY(auto d2color, make_pipeline<PipelineColor2D>("2dcolor.vert", "2dcolor.frag",
        va::vertex_color(logger)));

  DO_TRY(auto d2texture_wall,
      make_pipeline<PipelineTexture2D>("2dtexture.vert", "2dtexture.frag",
        va::vertex_uv2d(logger),
        texture::allocate_texture(logger, IMAGES::WALL)));

  DO_TRY(auto d2texture_container,
      make_pipeline<PipelineTexture2D>("2dtexture.vert", "2dtexture.frag",
        va::vertex_uv2d(logger),
        texture::allocate_texture(logger, IMAGES::CONTAINER)));

  auto const make_color = [](auto const& color) {
    constexpr auto ALPHA = 1.0f;
    return stlw::make_array<float>(color[0], color[1], color[2], ALPHA);
  };

  DO_TRY(auto d2wire,
      make_pipeline<PipelineWireframe2D>("wire.vert", "wire.frag",
        va::vertex_only(logger),
        make_color(LIST_OF_COLORS::PINK)));

  DO_TRY(auto d3color, make_pipeline<PipelineColor3D>("3dcolor.vert", "3dcolor.frag",
        va::vertex_color(logger)));

  DO_TRY(auto d3hashtag, make_pipeline<PipelineHashtag3D>("3d_hashtag.vert", "3d_hashtag.frag",
        va::vertex_color(logger)));

  DO_TRY(auto d3cube, make_pipeline<PipelineTextureCube3D>("3d_cubetexture.vert", "3d_cubetexture.frag",
        va::vertex_only(logger),
        texture::upload_3dcube_texture(logger,
          IMAGES::CUBE_FRONT,
          IMAGES::CUBE_RIGHT,
          IMAGES::CUBE_BACK,
          IMAGES::CUBE_LEFT,
          IMAGES::CUBE_TOP,
          IMAGES::CUBE_BOTTOM)));

  DO_TRY(auto d3house, make_pipeline<PipelineTexture3D>("3dtexture.vert", "3dtexture.frag",
        va::vertex_normal_uv3d(logger),
        texture::allocate_texture(logger, IMAGES::HOUSE)));

  DO_TRY(auto d3skybox, make_pipeline<PipelineSkybox3D>("3d_cubetexture.vert", "3d_cubetexture.frag",
        va::vertex_only(logger),
        texture::upload_3dcube_texture(logger,
          IMAGES::SB_FRONT,
          IMAGES::SB_RIGHT,
          IMAGES::SB_BACK,
          IMAGES::SB_LEFT,
          IMAGES::SB_TOP,
          IMAGES::SB_BOTTOM)));

  DO_TRY(auto d3wire, make_pipeline<PipelineWireframe3D>("3dwire.vert", "wire.frag",
        va::vertex_only(logger),
        make_color(LIST_OF_COLORS::PURPLE)));

  Pipeline2D d2{MOVE(d2color), MOVE(d2texture_wall), MOVE(d2texture_container), MOVE(d2wire)};
  Pipeline3D d3{MOVE(d3color), MOVE(d3hashtag), MOVE(d3cube), MOVE(d3house), MOVE(d3skybox), MOVE(d3wire)};
  return OpenglPipelines{MOVE(d2), MOVE(d3)};
}

} // ns opengl
