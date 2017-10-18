#pragma once
#include <string>
#include <tuple>
#include <vector>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <window/window.hpp>
#include <opengl/camera.hpp>
#include <opengl/factory.hpp>
#include <opengl/obj.hpp>
#include <opengl/skybox.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/burrito.hpp>
#include <stlw/random.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>

#include <game/boomhs/assets.hpp>
#include <game/boomhs/io_system.hpp>
#include <game/boomhs/randompos_system.hpp>

namespace game::boomhs
{

template <typename L>
struct game_state {
  bool quit = false;
  L &logger;
  window::dimensions const dimensions;
  stlw::float_generator rnum_generator;
  glm::mat4 projection;
  std::vector<::opengl::model*> MODELS;
  opengl::model skybox_model;
  opengl::model house_model;
  opengl::model terrain_model;
  opengl::camera camera;

  NO_COPY(game_state);
  NO_MOVE_ASSIGN(game_state);
public:
  MOVE_CONSTRUCTIBLE(game_state);
  game_state(L &l, window::dimensions const &d, stlw::float_generator &&fg,
      glm::mat4 &&pm)
    : logger(l)
    , dimensions(d)
    , rnum_generator(MOVE(fg))
    , projection(MOVE(pm))
    , camera(opengl::camera_factory::make_default(this->skybox_model))
  {
    camera.move_down(1);
  }
};

template<typename L, typename HW>
auto
make_state(L &logger, HW const& hw)
{
  auto const fheight = static_cast<GLfloat>(hw.h);
  auto const fwidth = static_cast<GLfloat>(hw.w);

  auto projection = glm::perspective(glm::radians(60.0f), (fwidth / fheight), 0.1f, 200.0f);
  stlw::float_generator rng;
  return game_state<L>(logger, hw, MOVE(rng), MOVE(projection));
}

class boomhs_game
{
  NO_COPY(boomhs_game);

public:
  boomhs_game() = default;
  MOVE_DEFAULT(boomhs_game);

  auto ecst_systems() const
  {
    return std::make_tuple(st::io_system, st::randompos_system);
  }

  template <typename LoopState, typename ShapeFactory, typename SHIT>
  void game_loop(LoopState &state, ShapeFactory &&sf, assets<SHIT> const& assets)
  {
    {
      auto const color = opengl::LIST_OF_COLORS::WHITE;
      glm::vec4 const vec4 = {color[0], color[1], color[2], 1.0};
      opengl::clear_screen(vec4);
    }

    using COLOR_ARRAY = std::array<float, 4>;
    auto constexpr multicolor_triangle =
        stlw::make_array<COLOR_ARRAY>(stlw::concat(opengl::LIST_OF_COLORS::RED, 1.0f),
                                      stlw::concat(opengl::LIST_OF_COLORS::GREEN, 1.0f),
                                      stlw::concat(opengl::LIST_OF_COLORS::BLUE, 1.0f));

    auto constexpr multicolor_rect =
        stlw::make_array<COLOR_ARRAY>(stlw::concat(opengl::LIST_OF_COLORS::RED, 1.0f),
                                      stlw::concat(opengl::LIST_OF_COLORS::GREEN, 1.0f),
                                      stlw::concat(opengl::LIST_OF_COLORS::BLUE, 1.0f),
                                      stlw::concat(opengl::LIST_OF_COLORS::YELLOW, 1.0f));

    auto const random_comp = [&]() { return state.rnum_generator.generate_0to1(); };
    auto const rc = [&]() {
      return glm::vec4{random_comp(), random_comp(), random_comp(), 1.0f};
    };

    auto &logger = state.logger;
    auto const height = 0.25f, width = 0.39f;
    auto cube_skybox = sf.d3.skybox.make_cube(logger, {GL_TRIANGLE_STRIP, state.skybox_model,
        {10.0f, 10.0f, 10.0f}});

    auto args = state.render_args();
    opengl::draw(args, cube_skybox);

    auto triangle_color = sf.d2.color.make_triangle(logger, {GL_TRIANGLES, *state.MODELS[9]},
        opengl::LIST_OF_COLORS::PINK);

    auto triangle_list_colors = sf.d2.color.make_triangle(logger, {GL_TRIANGLES, *state.MODELS[10]},
        multicolor_triangle);

    auto triangle_texture = sf.d2.texture_wall.make_triangle(logger, {GL_TRIANGLES, *state.MODELS[11]});

    auto triangle_wireframe = sf.d2.wireframe.make_triangle(logger, {GL_LINE_LOOP, *state.MODELS[12]});

    // 3d begin
    auto cube_texture = sf.d3.texture_cube.make_cube(logger, {GL_TRIANGLE_STRIP, *state.MODELS[100],
        {0.15f, 0.15f, 0.15f}});

    auto cube_color = sf.d3.color.make_cube(logger, {GL_TRIANGLE_STRIP, *state.MODELS[101],
        {0.25f, 0.25f, 0.25f}}, opengl::LIST_OF_COLORS::BLUE);

    auto cube_wf = sf.d3.wireframe.make_cube(logger, {GL_LINE_LOOP, *state.MODELS[102],
        {0.25f, 0.25f, 0.25f}});
    // 3d end

    auto rectangle_color = sf.d2.color.make_rectangle(logger, {GL_TRIANGLES, *state.MODELS[16]},
        opengl::LIST_OF_COLORS::YELLOW);

    auto rectangle_list_colors = sf.d2.color.make_rectangle(logger, {GL_TRIANGLE_STRIP, *state.MODELS[17],
        height, width}, multicolor_rect);

    auto rectangle_texture = sf.d2.texture_wall.make_rectangle(logger, {GL_TRIANGLE_STRIP, *state.MODELS[18],
        height, width});

    auto rectangle_wireframe = sf.d2.wireframe.make_rectangle(logger, {GL_LINE_LOOP, *state.MODELS[19], height,
        width});

    auto polygon_color = sf.d2.color.make_polygon(logger, {GL_TRIANGLE_FAN, *state.MODELS[20], 5},
        opengl::LIST_OF_COLORS::DARK_ORANGE);

    auto polygon_texture = sf.d2.texture_container.make_polygon(logger, {GL_TRIANGLE_FAN, *state.MODELS[21], 7});

    auto polygon_wireframe = sf.d2.wireframe.make_polygon(logger, {GL_LINE_LOOP, *state.MODELS[22], 7});

    //auto polygon_list_of_color = sf.d2.color.make_polygon(logger, {GL_TRIANGLE_FAN, *state.MODELS[23], 5},
        //multicolor_triangle});

    // first draw terrain
    auto cube_terrain = sf.d3.color.make_cube(logger, {GL_TRIANGLE_STRIP, state.terrain_model,
        {10.0f, 0.1f, 10.0f}}, opengl::LIST_OF_COLORS::SADDLE_BROWN);
    opengl::draw(args, cube_terrain);

    // now draw entities
    opengl::draw(args, cube_color);
    opengl::draw(args, cube_texture);
    opengl::draw(args, cube_wf);

    using COLOR_TRIANGLE = opengl::factories::pipeline_shape_pair<
      opengl::triangle<opengl::vertex_color_attributes, 3 * 8>, opengl::pipeline<opengl::color2d_context>>;

    {
      std::array<COLOR_TRIANGLE, 2> const arr = {
        sf.d2.color.make_triangle(logger, {GL_TRIANGLES, *state.MODELS[0]}, rc()),
        sf.d2.color.make_triangle(logger, {GL_TRIANGLES, *state.MODELS[1]}, rc())
      };
      for (auto const& it : arr) {
        opengl::draw(args, it);
      }
    }
    {
      std::array<COLOR_TRIANGLE, 2> const arr = {
        sf.d2.color.make_triangle(logger, {GL_TRIANGLES, *state.MODELS[2]}, rc()),
        sf.d2.color.make_triangle(logger, {GL_TRIANGLES, *state.MODELS[3]}, rc())
      };
      for (auto const& it : arr) {
        opengl::draw(args, it);
      }
    }
    {
      auto const t = sf.d2.color.make_triangle(logger, {GL_TRIANGLES, *state.MODELS[4]}, rc());
      opengl::draw(args, t);
    }
    {
      std::array<COLOR_TRIANGLE, 2> const arr = {
        sf.d2.color.make_triangle(logger, {GL_TRIANGLES, *state.MODELS[5]}, rc()),
        sf.d2.color.make_triangle(logger, {GL_TRIANGLES, *state.MODELS[6]}, rc())
      };
      for (auto const& it : arr) {
        opengl::draw(args, it);
      }
    }
    {
      std::array<COLOR_TRIANGLE, 2> const arr = {
        sf.d2.color.make_triangle(logger, {GL_TRIANGLES, *state.MODELS[7]}, rc()),
        sf.d2.color.make_triangle(logger, {GL_TRIANGLES, *state.MODELS[8]}, rc())
      };
      for (auto const& it : arr) {
        opengl::draw(args, it);
      }
    }

    // not draw 2d entities (last because we disable depth tests for these draw calls)
    opengl::draw(args, polygon_color);
    //opengl::draw(args, polygon_list_of_color);
    opengl::draw(args, rectangle_color);
    opengl::draw(args, rectangle_list_colors);
    opengl::draw(args, triangle_color);

    std::cerr << "start drawing house\n";
    opengl::draw(args, assets.house_uv);
    std::cerr << "finished drawing house\n";

    opengl::draw(args, triangle_texture);
    opengl::draw(args, rectangle_texture);
    opengl::draw(args, polygon_texture);
    opengl::draw(args, polygon_wireframe);
    opengl::draw(args, triangle_wireframe);
    opengl::draw(args, rectangle_wireframe);
  }
};

} // ns game::boomhs
