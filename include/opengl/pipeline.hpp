#pragma once
#include <opengl/context.hpp>
#include <opengl/shader_program.hpp>
#include <opengl/vertex_attribute.hpp>
#include <stlw/type_macros.hpp>

#include <opengl/obj.hpp>

namespace opengl
{

template<typename C>
class pipeline
{
  shader_program program_;
  C context_;
  vertex_attribute va_;
public:
  explicit pipeline(shader_program &&sp, C &&ctx, vertex_attribute &&v)
    : program_(MOVE(sp))
    , context_(MOVE(ctx))
    , va_(MOVE(v))
  {
  }

  auto const& va() const { return this->va_; }
  auto const& ctx() const { return this->context_; }

  auto& program_ref() { return this->program_; }

  using CTX = C;
  MOVE_CONSTRUCTIBLE_ONLY(pipeline);
};

struct pipeline2d
{
  pipeline<color2d_context> color;
  pipeline<texture2d_context> texture_wall;
  pipeline<texture2d_context> texture_container;
  pipeline<wireframe2d_context> wireframe;

  MOVE_CONSTRUCTIBLE_ONLY(pipeline2d);
};

struct pipeline3d
{
  pipeline<color3d_context> color;
  pipeline<wall_context> wall;
  pipeline<texture_3dcube_context> texture_3dcube;
  pipeline<texture3d_context> house;
  pipeline<skybox_context> skybox;
  pipeline<wireframe3d_context> wireframe;

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

template<typename C>
auto
make_pipeline(shader_program &&sp, C &&context, vertex_attribute &&va)
{
  return pipeline<C>{MOVE(sp), MOVE(context), MOVE(va)};
}

template<typename C>
stlw::result<pipeline<C>, std::string>
//auto
make_it(char const* vertex_s, char const* frag_s, vertex_attribute &&va, C &&ctx)
{
  shader_program_factory pf;
  DO_TRY(auto sp, pf.make(vertex_s, frag_s));
  return make_pipeline(MOVE(sp), MOVE(ctx), MOVE(va));
}

class opengl_pipelines_factory
{
  opengl_pipelines_factory() = delete;

public:
  template<typename L>
  static stlw::result<opengl_pipelines, std::string>
  make(L &logger)
  {
    // TODO: don't bother constructing opengl_contexts...
    DO_TRY(auto d2color, make_it("2dcolor.vert", "2dcolor.frag", va::vertex_color(logger),
          color2d_context{}));

    DO_TRY(auto d2texture_wall, make_it("2dtexture.vert", "2dtexture.frag", va::vertex_uv2d(logger),
          opengl_context2d::make_2dtexture(logger, IMAGES::WALL)));

    DO_TRY(auto d2texture_container, make_it("2dtexture.vert", "2dtexture.frag", va::vertex_uv2d(logger),
          opengl_context2d::make_2dtexture(logger, IMAGES::CONTAINER)));

    DO_TRY(auto d2wire, make_it("wire.vert", "wire.frag", va::vertex_only(logger),
          opengl_context2d::make_wireframe2d(logger, LIST_OF_COLORS::PINK)));

    DO_TRY(auto d3color, make_it("3dcolor.vert", "3dcolor.frag", va::vertex_color(logger),
          color3d_context{}));

    DO_TRY(auto d3wall, make_it("wall.vert", "wall.frag", va::vertex_color(logger),
          wall_context{}));

    DO_TRY(auto d3cube, make_it("3d_cubetexture.vert", "3d_cubetexture.frag", va::vertex_only(logger),
          opengl_context3d::make_texture3dcube(logger,
            IMAGES::CUBE_FRONT,
            IMAGES::CUBE_RIGHT,
            IMAGES::CUBE_BACK,
            IMAGES::CUBE_LEFT,
            IMAGES::CUBE_TOP,
            IMAGES::CUBE_BOTTOM)));

    DO_TRY(auto d3house, make_it("3dtexture.vert", "3dtexture.frag", va::vertex_normal_uv3d(logger),
          texture3d_context{texture::allocate_texture(logger, IMAGES::HOUSE)}));

    DO_TRY(auto d3skybox, make_it("3d_cubetexture.vert", "3d_cubetexture.frag", va::vertex_only(logger),
          opengl_context3d::make_skybox(logger,
            IMAGES::SB_FRONT,
            IMAGES::SB_RIGHT,
            IMAGES::SB_BACK,
            IMAGES::SB_LEFT,
            IMAGES::SB_TOP,
            IMAGES::SB_BOTTOM)));

    DO_TRY(auto d3wire, make_it("3dwire.vert", "wire.frag", va::vertex_only(logger),
          opengl_context3d::make_wireframe3d(logger, LIST_OF_COLORS::PURPLE)));

    pipeline2d d2{MOVE(d2color), MOVE(d2texture_wall), MOVE(d2texture_container), MOVE(d2wire)};
    pipeline3d d3{MOVE(d3color), MOVE(d3wall), MOVE(d3cube), MOVE(d3house), MOVE(d3skybox), MOVE(d3wire)};
    return opengl_pipelines{MOVE(d2), MOVE(d3)};
  }
};

} // ns opengl
