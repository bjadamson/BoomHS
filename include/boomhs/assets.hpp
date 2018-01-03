#pragma once
#include <stlw/type_macros.hpp>
#include <opengl/obj.hpp>
#include <opengl/factory.hpp>

namespace boomhs {

struct Objs {
  opengl::obj house;
  opengl::obj hashtag;
  opengl::obj at;
  opengl::obj plus;
  opengl::obj arrow;

  // Alphabet
  opengl::obj O;
  opengl::obj T;

  MOVE_CONSTRUCTIBLE_ONLY(Objs);
};

struct GpuHandles {
  opengl::DrawInfo house;
  opengl::DrawInfo hashtag;
  opengl::DrawInfo at;
  opengl::DrawInfo plus;
  opengl::DrawInfo arrow;

  // Alphabet
  opengl::DrawInfo O;
  opengl::DrawInfo T;

  opengl::DrawInfo light;

  opengl::DrawInfo x_axis_arrow;
  opengl::DrawInfo y_axis_arrow;
  opengl::DrawInfo z_axis_arrow;

  opengl::DrawInfo cube_skybox;
  opengl::DrawInfo cube_textured;
  opengl::DrawInfo cube_colored;
  opengl::DrawInfo cube_wireframe;

  opengl::DrawInfo terrain;
  opengl::DrawInfo tilemap;

  MOVE_CONSTRUCTIBLE_ONLY(GpuHandles);
};

struct Assets {
  Objs objects;
  GpuHandles handles;

  MOVE_CONSTRUCTIBLE_ONLY(Assets);
};

} // ns boomhs
