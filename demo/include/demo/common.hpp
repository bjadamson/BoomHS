#pragma once
#include <boomhs/color.hpp>
#include <boomhs/components.hpp>
#include <boomhs/math.hpp>
#include <boomhs/mouse.hpp>
#include <boomhs/random.hpp>
#include <boomhs/transform.hpp>
#include <boomhs/viewport.hpp>

#include <opengl/draw_info.hpp>
#include <opengl/gpu.hpp>
#include <opengl/shader.hpp>
#include <opengl/vertex_attribute.hpp>

#include <vector>

namespace boomhs
{
struct CameraMatrices;
class  RNG;
} // namespace boomhs

namespace opengl
{
struct DrawState;
} // namespace opengl

namespace demo
{

enum ScreenSector
{
  LEFT_TOP = 0,
  RIGHT_TOP,
  LEFT_BOTTOM,
  RIGHT_BOTTOM,
  MAX
};

struct MouseCursorInfo
{
  ScreenSector sector;
  boomhs::MouseClickPositions click_positions;
};


inline auto
make_perspective_rect_gpuhandle(common::Logger& logger, boomhs::RectFloat const& rect,
                                opengl::VertexAttribute const& va)
{
  using namespace boomhs;

  auto buffer = RectBuilder{rect}.build();
  return OG::copy_rectangle(logger, va, buffer);
}

inline auto
make_perspective_rect(boomhs::Viewport const& viewport, boomhs::RNG& rng)
{
  auto const gen = [&rng](auto const low, auto const high) { return rng.gen_float_range(low, high); };

  auto const left  = gen(viewport.left(), viewport.right());
  auto const right = gen(left, viewport.right());

  auto const top    = gen(viewport.top(), viewport.bottom());
  auto const bottom = gen(top, viewport.bottom());

  return boomhs::RectFloat{left, top, right, bottom};
};

class CubeEntity
{
  boomhs::Cube      cube_;
  boomhs::Transform tr_;
  opengl::DrawInfo  di_;
public:
  MOVE_DEFAULT_ONLY(CubeEntity);

  CubeEntity(boomhs::Cube&& cube, boomhs::Transform&& tr, opengl::DrawInfo&& di)
      : cube_(MOVE(cube))
      , tr_(MOVE(tr))
      , di_(MOVE(di))
  {
  }

  bool selected = false;

  auto const& cube() const { return cube_; }
  auto& cube() { return cube_; }

  auto const& transform() const { return tr_; }
  auto& transform() { return tr_; }

  auto const& draw_info() const { return di_; }
  auto& draw_info() { return di_; }
};
using CubeEntities = std::vector<CubeEntity>;

void
select_cubes_under_user_drawn_rect(common::Logger&, boomhs::RectFloat const&,
                                   CubeEntities&, boomhs::ProjMatrix const&,
                                   boomhs::ViewMatrix const&, boomhs::Viewport const&);



void
draw_bbox(common::Logger&, boomhs::CameraMatrices const&, opengl::ShaderProgram&,
          boomhs::Transform const&, opengl::DrawInfo&, boomhs::Color const&, opengl::DrawState&);

void
draw_bboxes(common::Logger&, boomhs::CameraMatrices const&, CubeEntities&, opengl::ShaderProgram&,
            opengl::DrawState&);

opengl::ShaderProgram
make_wireframe_program(common::Logger&);

boomhs::Cube
make_cube(boomhs::RNG&, float, float);

CubeEntities
gen_cube_entities(common::Logger&, size_t, boomhs::ScreenSize const&, opengl::ShaderProgram const&,
                  boomhs::RNG &, bool);

} // namespace demo
