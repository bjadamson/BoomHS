#include <demo/common.hpp>
#include <boomhs/collision.hpp>
#include <boomhs/frame.hpp>

#include <opengl/gpu.hpp>
#include <opengl/renderer.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

auto
make_bbox(common::Logger& logger, ShaderProgram const& sp, Cube const& cr)
{
  auto const vertices = VertexFactory::build_cube(cr.min, cr.max);
  return OG::copy_cube_wireframe_gpu(logger, vertices, sp.va());
}

} // namespace

namespace demo
{

void
select_cubes_under_user_drawn_rect(common::Logger& logger, RectFloat const& mouse_rect,
                                   CubeEntities& cube_ents, ProjMatrix const& proj, ViewMatrix const& view,
                                   Viewport const& viewport)
{
  namespace sc       = math::space_conversions;
  auto const cxx4l   = [](auto const& p) { return glm::vec3{p.x, p.y, 0}; };
  auto const vp_rect = viewport.rect_float();

  // Determine whether a cube projected onto the given plane and another rectangle overlap.
  auto const cube_mouserect_overlap = [&](auto const& cube_entity) {
    auto const& cube = cube_entity.cube();
    auto tr          = cube_entity.transform();

    // Take the Cube in Object space, and create a rectangle from the x/z coordinates.
    auto xz = cube.xy_rect();

    auto const lt_cube = cxx4l(xz.left_top());
    auto const rb_cube = cxx4l(xz.right_bottom());

    auto const model = tr.model_matrix();

    auto const to_screen = [&](auto const& cube) {
      return sc::object_to_screen(cube, model, proj, view, vp_rect);
    };
    auto const lt_cube_ss = to_screen(lt_cube);
    auto const rb_cube_ss = to_screen(rb_cube);
    xz = RectFloat{lt_cube_ss, rb_cube_ss};

    RectTransform const rect_tr{xz, tr};
    return collision::overlap(mouse_rect, rect_tr, proj, view);
  };

  for (auto &ce : cube_ents) {
    ce.selected = cube_mouserect_overlap(ce);
  }
}

void
draw_bbox(common::Logger& logger, CameraMatrices const& cm, ShaderProgram& sp,
          Transform const& tr, DrawInfo& dinfo, Color const& color, DrawState& ds)
{
  auto const model_matrix = tr.model_matrix();

  BIND_UNTIL_END_OF_SCOPE(logger, sp);
  shader::set_uniform(logger, sp, "u_wirecolor", color);

  BIND_UNTIL_END_OF_SCOPE(logger, dinfo);
  auto const camera_matrix = cm.proj * cm.view;
  OR::set_mvpmatrix(logger, camera_matrix, model_matrix, sp);
  OR::draw(logger, ds, GL_LINES, sp, dinfo);
}

void
draw_bboxes(common::Logger& logger, CameraMatrices const& cm,
            CubeEntities& cube_ents, ShaderProgram& sp,
            DrawState& ds)
{
  for (auto &cube_tr : cube_ents) {
    auto const& tr      = cube_tr.transform();
    auto &dinfo         = cube_tr.draw_info();
    bool const selected = cube_tr.selected;

    auto const& wire_color = selected ? LOC4::LIGHT_GOLDENROD_YELLOW : LOC4::DARKRED;
    draw_bbox(logger, cm, sp, tr, dinfo, wire_color, ds);
  }
}

ShaderProgram
make_wireframe_program(common::Logger& logger)
{
  std::vector<AttributePointerInfo> const apis{{
    AttributePointerInfo{0, GL_FLOAT, AttributeType::POSITION, 3}
  }};

  auto va = opengl::make_vertex_attribute(apis);
  return make_shader_program(logger, "wireframe.vert", "wireframe.frag", MOVE(va))
    .expect_moveout("Error loading wireframe shader program");
}

Cube
make_cube(RNG& rng, bool const is_2d)
{
  float constexpr MIN = 0, MAX = 200;
  static_assert(MIN < MAX, "MIN must be atleast one less than MAX");

  auto const gen = [&rng]() { return 100; };//rng.gen_float_range(MIN + 1, MAX); };

  glm::vec3 min, max;
  if (is_2d) {
    min = glm::vec3{-50, -50, 0};
    max = glm::vec3{50, 50, 0};
  }
  else {
    min = glm::vec3{MIN, MIN, MIN};
    max = glm::vec3{gen(), gen(), gen()};
  }
  return Cube{min, max};
}

CubeEntities
gen_cube_entities(common::Logger& logger, size_t const num_cubes, ScreenSize const& ss,
                  ShaderProgram const& sp, RNG &rng, bool const is_2d)
{
  auto const gen = [&rng](auto const& l, auto const& h) { return rng.gen_float_range(l, h); };
  auto const gen_between_0_and = [&gen](auto const& max) { return gen(0, max); };
  auto const gen_tr = [&]() {
    auto const x = 100.0f;//gen_between_0_and(ss.width);
    auto const y = 100.0f;//gen_between_0_and(ss.height);

    return is_2d
      ? glm::vec3{x, y, 0}
      : glm::vec3{x, 0, y};
  };

  CubeEntities cube_ents;
  FOR(i, num_cubes) {
    auto cube = demo::make_cube(rng, is_2d);
    auto tr = gen_tr();
    auto di = make_bbox(logger, sp, cube);
    cube_ents.emplace_back(MOVE(cube), MOVE(tr), MOVE(di));

    auto& back = cube_ents.back();
    back.transform().rotate_degrees(5.0f, math::constants::Z_UNIT_VECTOR);
  }
  return cube_ents;
}

} // namespace demo
