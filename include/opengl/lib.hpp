#pragma once
#include <stlw/burrito.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>

#include <opengl/camera.hpp>
#include <opengl/factory.hpp>
#include <opengl/renderer.hpp>
#include <opengl/vao.hpp>

namespace opengl
{

template<typename C>
class pipeline
{
  shader_program program_;
  C vao_;
  vertex_attribute va_;
public:
  explicit pipeline(shader_program &&sp, C &&ctx, vertex_attribute &&v)
    : program_(MOVE(sp))
    , vao_(MOVE(ctx))
    , va_(MOVE(v))
  {
  }

  auto const& va() const { return this->va_; }
  auto const& ctx() const { return this->vao_; }

  auto& program_ref() { return this->program_; }

  using CTX = C;
  MOVE_CONSTRUCTIBLE_ONLY(pipeline);
};

struct pipeline2d
{
  pipeline<color2d_vao> color;
  pipeline<texture2d_vao> texture_wall;
  pipeline<texture2d_vao> texture_container;
  pipeline<wireframe2d_vao> wireframe;

  MOVE_CONSTRUCTIBLE_ONLY(pipeline2d);
};

struct pipeline3d
{
  pipeline<color3d_vao> color;
  pipeline<hashtag3d_vao> hashtag;
  pipeline<texture_3dcube_vao> texture_cube;
  pipeline<texture3d_vao> house;
  pipeline<skybox_vao> skybox;
  pipeline<wireframe3d_vao> wireframe;

  MOVE_CONSTRUCTIBLE_ONLY(pipeline3d);
};

struct opengl_pipelines
{
  pipeline2d d2;
  pipeline3d d3;

  MOVE_CONSTRUCTIBLE_ONLY(opengl_pipelines);
  explicit opengl_pipelines(pipeline2d &&p2d, pipeline3d &&p3d)
    : d2(MOVE(p2d))
    , d3(MOVE(p3d))
  {
  }
};

class lib_factory
{
  lib_factory() = delete;

  template<typename C>
  static stlw::result<pipeline<C>, std::string>
  make_pipeline(char const* vertex_s, char const* frag_s, vertex_attribute &&va, C &&vao)
  {
    shader_program_factory pf;
    DO_TRY(auto sp, pf.make(vertex_s, frag_s));
    return pipeline<C>{MOVE(sp), MOVE(vao), MOVE(va)};
  }

public:
  template<typename L>
  static stlw::result<opengl_pipelines, std::string>
  make_opengl_factories(L &logger)
  {
    // TODO: don't bother constructing opengl_vaos...
    DO_TRY(auto d2color, make_pipeline("2dcolor.vert", "2dcolor.frag",
          va::vertex_color(logger), color2d_vao{}));

    DO_TRY(auto d2texture_wall,
        make_pipeline("2dtexture.vert", "2dtexture.frag",
          va::vertex_uv2d(logger),
          opengl_vao2d::make_2dtexture(logger, IMAGES::WALL)));

    DO_TRY(auto d2texture_container,
        make_pipeline("2dtexture.vert", "2dtexture.frag",
          va::vertex_uv2d(logger),
          opengl_vao2d::make_2dtexture(logger, IMAGES::CONTAINER)));

    DO_TRY(auto d2wire,
        make_pipeline("wire.vert", "wire.frag",
          va::vertex_only(logger),
          opengl_vao2d::make_wireframe2d(logger, LIST_OF_COLORS::PINK)));

    DO_TRY(auto d3color, make_pipeline("3dcolor.vert", "3dcolor.frag",
          va::vertex_color(logger),
          color3d_vao{}));

    DO_TRY(auto d3hashtag, make_pipeline("3d_hashtag.vert", "3d_hashtag.frag",
          va::vertex_color(logger),
          hashtag3d_vao{}));

    DO_TRY(auto d3cube, make_pipeline("3d_cubetexture.vert", "3d_cubetexture.frag",
          va::vertex_only(logger),
          opengl_vao3d::make_texture3dcube(logger,
            IMAGES::CUBE_FRONT,
            IMAGES::CUBE_RIGHT,
            IMAGES::CUBE_BACK,
            IMAGES::CUBE_LEFT,
            IMAGES::CUBE_TOP,
            IMAGES::CUBE_BOTTOM)));

    DO_TRY(auto d3house, make_pipeline("3dtexture.vert", "3dtexture.frag",
          va::vertex_normal_uv3d(logger),
          texture3d_vao{texture::allocate_texture(logger, IMAGES::HOUSE)}));

    DO_TRY(auto d3skybox, make_pipeline("3d_cubetexture.vert", "3d_cubetexture.frag",
          va::vertex_only(logger),
          opengl_vao3d::make_skybox(logger,
            IMAGES::SB_FRONT,
            IMAGES::SB_RIGHT,
            IMAGES::SB_BACK,
            IMAGES::SB_LEFT,
            IMAGES::SB_TOP,
            IMAGES::SB_BOTTOM)));

    DO_TRY(auto d3wire, make_pipeline("3dwire.vert", "wire.frag",
          va::vertex_only(logger),
          opengl_vao3d::make_wireframe3d(logger, LIST_OF_COLORS::PURPLE)));

    pipeline2d d2{MOVE(d2color), MOVE(d2texture_wall), MOVE(d2texture_container), MOVE(d2wire)};
    pipeline3d d3{MOVE(d3color), MOVE(d3hashtag), MOVE(d3cube), MOVE(d3house), MOVE(d3skybox), MOVE(d3wire)};
    return opengl_pipelines{MOVE(d2), MOVE(d3)};
  }
};

} // ns opengl
