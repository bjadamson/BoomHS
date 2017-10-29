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

template<typename C>
auto
make_pipeline(shader_program &&sp, C &&context, vertex_attribute &&va)
{
  return pipeline<C>{MOVE(sp), MOVE(context), MOVE(va)};
}

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

struct opengl_pipelines_factory
{
  opengl_pipelines_factory() = delete;

  template<typename L>
  static stlw::result<opengl_pipelines, std::string>
  make(L &logger, opengl_contexts &&contexts)
  {
    shader_program_factory pf;
    va_factory vf;
    auto va0 = vf.make_vertex_color(logger);
    DO_TRY(auto p0, pf.make("2dcolor.vert", "2dcolor.frag"));
    auto pipe0 = make_pipeline(MOVE(p0), MOVE(contexts.d2.color), MOVE(va0));

    auto va1 = vf.make_vertex_uv2d(logger);
    DO_TRY(auto p1, pf.make("2dtexture.vert", "2dtexture.frag"));
    auto pipe1 = make_pipeline(MOVE(p1), MOVE(contexts.d2.texture_wall), MOVE(va1));

    auto va2 = vf.make_vertex_uv2d(logger);
    DO_TRY(auto p2, pf.make("2dtexture.vert", "2dtexture.frag"));
    auto pipe2 = make_pipeline(MOVE(p2), MOVE(contexts.d2.texture_container), MOVE(va2));

    auto va3 = vf.make_vertex_only(logger);
    DO_TRY(auto p3, pf.make("wire.vert", "wire.frag"));
    auto pipe3 = make_pipeline(MOVE(p3), MOVE(contexts.d2.wireframe), MOVE(va3));

    auto va4 = vf.make_vertex_color(logger);
    DO_TRY(auto p4, pf.make("3dcolor.vert", "3dcolor.frag"));
    auto pipe4 = make_pipeline(MOVE(p4), MOVE(contexts.d3.color), MOVE(va4));

    auto va5 = vf.make_vertex_color(logger);
    DO_TRY(auto p5, pf.make("wall.vert", "wall.frag"));
    auto pipe5 = make_pipeline(MOVE(p5), MOVE(contexts.d3.wall), MOVE(va5));

    auto va6 = vf.make_vertex_only(logger);
    DO_TRY(auto p6, pf.make("3d_cubetexture.vert", "3d_cubetexture.frag"));
    auto pipe6 = make_pipeline(MOVE(p6), MOVE(contexts.d3.texture), MOVE(va6));

    auto va7 = vf.make_vertex_normal_uv3d(logger); //(not using normals in shaders atm)
    DO_TRY(auto p7, pf.make("3dtexture.vert", "3dtexture.frag"));
    auto pipe7 = make_pipeline(MOVE(p7), MOVE(contexts.d3.house_texture), MOVE(va7));

    auto va8 = vf.make_vertex_only(logger);
    DO_TRY(auto p8, pf.make("3d_cubetexture.vert", "3d_cubetexture.frag"));
    auto pipe8 = make_pipeline(MOVE(p8), MOVE(contexts.d3.skybox), MOVE(va8));

    auto va9 = vf.make_vertex_only(logger);
    DO_TRY(auto p9, pf.make("3dwire.vert", "wire.frag"));
    auto pipe9 = make_pipeline(MOVE(p9), MOVE(contexts.d3.wireframe), MOVE(va9));

    pipeline2d d2{MOVE(pipe0), MOVE(pipe1), MOVE(pipe2), MOVE(pipe3)};
    pipeline3d d3{MOVE(pipe4), MOVE(pipe5), MOVE(pipe6), MOVE(pipe7), MOVE(pipe8), MOVE(pipe9)};
    return opengl_pipelines{MOVE(d2), MOVE(d3)};
  }
};

} // ns opengl
