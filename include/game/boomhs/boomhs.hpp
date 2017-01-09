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

  template <typename LoopState, typename R, typename ShapeFactory>
  void game_loop(LoopState &state, R &renderer, ShapeFactory &&sf)
  {
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
    // first, pick random shape
    auto const random_mode = [&]() {
      return static_cast<opengl::draw_mode>(opengl::draw_mode::TRIANGLES);
    };

    auto const random_comp = [&]() { return state.rnum_generator.generate_0to1(); };
    auto const rc = [&]() {
      return glm::vec4{random_comp(), random_comp(), random_comp(), 1.0f};
    };

    auto &r = renderer;
    auto const height = 0.25f, width = 0.39f;
    auto cube_skybox = sf.d3.skybox.make_cube({opengl::draw_mode::TRIANGLE_STRIP, state.skybox_model,
        {10.0f, 10.0f, 10.0f}});

    auto cube_terrain = sf.d3.color.make_cube({opengl::draw_mode::TRIANGLE_STRIP, state.terrain_model,
        {10.0f, 0.1f, 10.0f}}, opengl::LIST_OF_COLORS::SADDLE_BROWN);

    r.begin();
    auto args = state.render_args();
    r.draw(args, MOVE(cube_skybox));

    auto triangle_color = sf.d2.color.make_triangle({opengl::draw_mode::TRIANGLES, *state.MODELS[9]},
        opengl::LIST_OF_COLORS::PINK);

    auto triangle_list_colors = sf.d2.color.make_triangle({opengl::draw_mode::TRIANGLES, *state.MODELS[10]},
        multicolor_triangle);

    auto triangle_texture = sf.d2.texture_wall.make_triangle({opengl::draw_mode::TRIANGLES, *state.MODELS[11]});

    auto triangle_wireframe = sf.d2.wireframe.make_triangle({opengl::draw_mode::LINE_LOOP, *state.MODELS[12]});

    // 3d begin
    auto cube_texture = sf.d3.texture.make_cube({opengl::draw_mode::TRIANGLE_STRIP, *state.MODELS[100],
        {0.15f, 0.15f, 0.15f}});

    auto cube_color = sf.d3.color.make_cube({opengl::draw_mode::TRIANGLE_STRIP, *state.MODELS[101],
        {0.25f, 0.25f, 0.25f}}, opengl::LIST_OF_COLORS::BLUE);

    auto cube_wf = sf.d3.wireframe.make_cube({opengl::draw_mode::LINE_LOOP, *state.MODELS[102],
        {0.25f, 0.25f, 0.25f}});


    std::cerr << "load_house\n";
    auto house_mesh = opengl::load_mesh("assets/house.obj");
    std::cerr << "make_mesh\n";
    auto house_color = sf.d3.color.make_mesh({opengl::draw_mode::TRIANGLE_STRIP, *state.MODELS[103],
        house_mesh});
    // 3d end

    auto rectangle_color = sf.d2.color.make_rectangle({opengl::draw_mode::TRIANGLE_STRIP, *state.MODELS[16]},
        opengl::LIST_OF_COLORS::YELLOW);

    auto rectangle_list_colors = sf.d2.color.make_rectangle({opengl::draw_mode::TRIANGLE_STRIP, *state.MODELS[17],
        height, width}, multicolor_rect);

    auto rectangle_texture = sf.d2.texture_wall.make_rectangle({opengl::draw_mode::TRIANGLE_STRIP, *state.MODELS[18],
        height, width});

    auto rectangle_wireframe = sf.d2.wireframe.make_rectangle({opengl::draw_mode::LINE_LOOP, *state.MODELS[19], height,
        width});

    auto polygon_color = sf.d2.color.make_polygon({opengl::draw_mode::TRIANGLE_FAN, *state.MODELS[20], 5},
        opengl::LIST_OF_COLORS::DARK_ORANGE);

    auto polygon_texture = sf.d2.texture_container.make_polygon({opengl::draw_mode::TRIANGLE_FAN, *state.MODELS[21], 7});

    auto polygon_wireframe = sf.d2.wireframe.make_polygon({opengl::draw_mode::LINE_LOOP, *state.MODELS[22], 7});

    //auto polygon_list_of_color = d2c.make_polygon(opengl::draw_mode::TRIANGLE_FAN, *zp1, 5,
        //multicolor_triangle);

    // first draw terrain
    r.draw(args, cube_terrain);

    // now draw entities
    r.draw(args, cube_color);
    r.draw(args, cube_texture);
    r.draw(args, cube_wf);

    {
      //std::array<opengl::triangle<opengl::vertex_color_attributes>, 2> const arr = {
        //sf.d2.color.make_triangle({random_mode(), *state.MODELS[0]}, rc()),
        //sf.d2.color.make_triangle({random_mode(), *state.MODELS[1]}, rc())
      //};
      //r.draw(args, arr);
    }
    {
      //std::array<opengl::triangle<opengl::vertex_color_attributes>, 2> arr = {
        //sf.d2.color.make_triangle({random_mode(), *state.MODELS[2]}, rc()),
        //sf.d2.color.make_triangle({random_mode(), *state.MODELS[3]}, rc())
      //};
      //r.draw(args, MOVE(arr));
    }
    {
      //r.draw(args, std::make_tuple(
            //sf.d2.color.make_triangle({random_mode(), *state.MODELS[4]}, rc())
            //));
    }
    {
      //r.draw(args,
            //sf.d2.color.make_triangle({random_mode(), *state.MODELS[5]}, rc()),
            //sf.d2.color.make_triangle({random_mode(), *state.MODELS[6]}, rc())
            //);
    }
    {
      //std::vector<opengl::triangle<opengl::vertex_color_attributes>> vec;
      //vec.emplace_back(sf.d2.color.make_triangle({random_mode(), *state.MODELS[7]}, rc()));
      //vec.emplace_back(sf.d2.color.make_triangle({random_mode(), *state.MODELS[8]}, rc()));

      //r.draw(args, MOVE(vec));
    }

    // not draw 2d entities (last because we disable depth tests for these draw calls)
    //r.draw(args, triangle_color, triangle_list_colors, MOVE(polygon_color),
                                           //rectangle_color, rectangle_list_colors
                                           // polygon_list_of_color,
                                           //);

    r.draw(args, house_color);
    //r.draw(args, triangle_texture, rectangle_texture);
    //r.draw(args, MOVE(polygon_texture));
    //r.draw(args, MOVE(polygon_wireframe), triangle_wireframe, rectangle_wireframe);
    r.end();
  }
};

} // ns game::boomhs
