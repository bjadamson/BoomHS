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
                                   glm::ivec2 const& vpgrid_size, CubeEntities& cube_ents,
                                   ProjMatrix const& proj, ViewMatrix const& view)
{
  namespace sc = math::space_conversions;

  // Determine whether a cube projected onto the XZ plane and another rectangle overlap.
  auto const cube_mouserect_overlap = [&](auto const& cube_entity) {
    auto const& cube = cube_entity.cube();
    auto tr          = cube_entity.transform();

    // Take the Cube in Object space, and create a rectangle from the x/z coordinates.
    auto xz = cube.xz_rect();
    xz = sc::screen_to_viewport(xz, vpgrid_size);

    // Translate the rectangle from Object space to world space.
    auto const xm = tr.translation.x / vpgrid_size.x;
    auto const zm = tr.translation.z / vpgrid_size.y;
    xz.move(xm, zm);

    /*
    auto const model = tr.model_matrix();
    RectFloat const vp{0, 0, 1024/2, 768/2};

    auto const cxx4l = [](auto const& p) { return glm::vec3{p.x, 0.0f, p.y}; };

    auto const lt_cube = cxx4l(xz.left_top());
    auto const rb_cube = cxx4l(xz.right_bottom());

    auto const lt_cube_ss = sc::object_to_screen(lt_cube, model, proj, view, vp);
    auto const rb_cube_ss = sc::object_to_screen(rb_cube, model, proj, view, vp);
    //LOG_ERROR_SPRINTF("lt_cube_ss: %s, rb_cube_ss: %s",
                      //glm::to_string(lt_cube_ss),
                      //glm::to_string(rb_cube_ss));

    auto const lt_mouse = cxx4l(mouse_rect.left_top());
    auto const rb_mouse = cxx4l(mouse_rect.right_bottom());

    auto const lt_mouse_ss = sc::object_to_screen(lt_mouse, model, proj, view, vp);
    auto const rb_mouse_ss = sc::object_to_screen(rb_mouse, model, proj, view, vp);

    //LOG_ERROR_SPRINTF("lt_mouse_ss: %s, rb_mouse_ss: %s",
                      //glm::to_string(lt_mouse_ss),
                      //glm::to_string(rb_mouse_ss));
    //LOG_ERROR_SPRINTF("mouse (object space?): %s", mouse_rect.to_string());

    //LOG_ERROR("");
    xz = RectFloat{lt_cube_ss.x, lt_cube_ss.z, rb_cube_ss.x, rb_cube_ss.z};
    return collision::overlap_axis_aligned(xz, mouse_rect);
    */

    // Combine the transform and rectangle into a single structure.
    RectTransform const rect_tr{xz, tr};


    // Compute whether the rectangle (converted from the cube) and the rectangle from the user
    // clicking and dragging are overlapping.
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

    auto const& wire_color = selected ? LOC4::BLUE : LOC4::RED;
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
make_cube(RNG& rng)
{
  float constexpr MIN = -100, MAX = 100;
  static_assert(MIN < MAX, "MIN must be atleast one less than MAX");

  auto const gen = [&rng]() { return rng.gen_float_range(MIN + 1, MAX); };
  glm::vec3 const min{MIN, MIN, MIN};
  glm::vec3 const max{gen(), gen(), gen()};

  //glm::vec3 const min{0}, max{600, 0, 600};
  return Cube{min, max};
}

CubeEntities
gen_cube_entities(common::Logger& logger, size_t const num_cubes, ScreenSize const& ss,
                  ShaderProgram const& sp, RNG &rng)
{
  auto const gen = [&rng](auto const& l, auto const& h) { return rng.gen_float_range(l, h); };
  auto const gen_low_x = [&gen, &ss]() { return gen(0, ss.width); };
  auto const gen_low_z = [&gen, &ss]() { return gen(0, ss.height); };
  auto const gen_tr = [&]() { return glm::vec3{0, 0, 0}; };
  //glm::vec3{gen_low_x(), 0, gen_low_z()}; };

  CubeEntities cube_ents;
  FOR(i, num_cubes) {
    auto cube = demo::make_cube(rng);
    auto tr = gen_tr();
    auto di = make_bbox(logger, sp, cube);
    cube_ents.emplace_back(MOVE(cube), MOVE(tr), MOVE(di));
  }
  return cube_ents;
}

} // namespace demo
