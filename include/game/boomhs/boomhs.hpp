#pragma once
#include <string>
#include <tuple>
#include <vector>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <window/window.hpp>
#include <gfx/camera.hpp>
#include <gfx/factory.hpp>
#include <gfx/skybox.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/burrito.hpp>
#include <stlw/random.hpp>
#include <stlw/result.hpp>
#include <stlw/type_ctors.hpp>

#include <game/boomhs/io_system.hpp>
#include <game/boomhs/randompos_system.hpp>
#include <game/data_types.hpp>

namespace game::boomhs
{

template <typename L>
struct game_state {
  bool quit = false;
  L &logger;
  window::dimensions const dimensions;
  stlw::float_generator rnum_generator;
  glm::mat4 projection;
  std::vector<::gfx::model*> MODELS;
  gfx::model skybox_model;
  gfx::camera camera;

  NO_COPY(game_state);
  NO_MOVE_ASSIGN(game_state);
public:
  MOVE_CONSTRUCTIBLE(game_state);
  game_state(L &l, window::dimensions const &d, stlw::float_generator &&fg,
      glm::mat4 &&pm)
    : logger(l)
    , dimensions(d)
    , rnum_generator(std::move(fg))
    , projection(std::move(pm))
    , camera(gfx::camera_factory::make_default(this->skybox_model))
  {
  }
};

template<typename L, typename HW>
auto
make_state(L &logger, HW const& hw)
{
  auto const fheight = static_cast<GLfloat>(hw.h);
  auto const fwidth = static_cast<GLfloat>(hw.w);

  logger.error(fmt::sprintf("fheight '%f', fwidth '%f'", fheight, fwidth));
  auto projection = glm::perspective(45.0f, (fwidth / fheight), 0.1f, 10000.0f);
  stlw::float_generator rng;
  return game_state<L>(logger, hw, std::move(rng), std::move(projection));
}

template<typename D, typename C>
struct opengl_shape
{
  D data;
  C &context;

  opengl_shape(D &&d, C &c) : data(std::move(d)), context(c) {}
  MOVE_CONSTRUCTIBLE_ONLY(opengl_shape);
};

template<typename D, typename C>
auto ms(D &&d, C &c)
{
  return opengl_shape<D, C>(std::move(d), c);
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

  template <typename LoopState, typename R>
  void game_loop(LoopState &state, R &renderer)
  {
    using COLOR_ARRAY = std::array<float, 4>;
    auto constexpr multicolor_triangle =
        stlw::make_array<COLOR_ARRAY>(stlw::concat(gfx::LIST_OF_COLORS::RED, 1.0f),
                                      stlw::concat(gfx::LIST_OF_COLORS::GREEN, 1.0f),
                                      stlw::concat(gfx::LIST_OF_COLORS::BLUE, 1.0f));

    auto constexpr multicolor_rect =
        stlw::make_array<COLOR_ARRAY>(stlw::concat(gfx::LIST_OF_COLORS::RED, 1.0f),
                                      stlw::concat(gfx::LIST_OF_COLORS::GREEN, 1.0f),
                                      stlw::concat(gfx::LIST_OF_COLORS::BLUE, 1.0f),
                                      stlw::concat(gfx::LIST_OF_COLORS::YELLOW, 1.0f));
    namespace gfx = gfx;

    // first, pick random shape
    auto const random_mode = [&]() {
      return static_cast<gfx::draw_mode>(gfx::draw_mode::TRIANGLES);
    };

    auto const random_comp = [&]() { return state.rnum_generator.generate_0to1(); };
    auto const rc = [&]() {
      return glm::vec4{random_comp(), random_comp(), random_comp(), 1.0f};
    };

    gfx::shape_factory sf;

    auto const height = 0.25f, width = 0.39f;
    auto cube_skybox = sf.make_textured_cube({gfx::draw_mode::TRIANGLE_STRIP, state.skybox_model,
        {15.0f, 15.0f, 15.0f}}, gfx::uv_t{});

    auto &rd = renderer;
    auto &r = rd;
    auto &d2 = r.gfx_engine.d2;
    auto &d3 = r.gfx_engine.d3;
    rd.begin();
    auto x = ms(std::move(cube_skybox), d3.skybox);
    auto args = state.render_args();
    //r.draw_special(args, std::move(x));
    r.draw(args, d3.skybox, std::move(cube_skybox));
    /*
    {
      std::array<gfx::triangle<gfx::vertex_color_attributes>, 2> const arr = {
        gfx::triangle_factory::make({random_mode(), *state.MODELS[0]}, gfx::color_t{}, rc()),
        gfx::triangle_factory::make({random_mode(), *state.MODELS[1]}, gfx::color_t{}, rc())
      };
      r.draw(args, d2.color, arr);
    }
    {
      std::array<gfx::triangle<gfx::vertex_color_attributes>, 2> arr = {
        gfx::triangle_factory::make({random_mode(), *state.MODELS[2]}, gfx::color_t{}, rc()),
        gfx::triangle_factory::make({random_mode(), *state.MODELS[3]}, gfx::color_t{}, rc())
      };
      r.draw(args, d2.color, std::move(arr));
    }
    {
      r.draw(args, d2.color, std::make_tuple(
            gfx::triangle_factory::make({random_mode(), *state.MODELS[4]}, gfx::color_t{}, rc())
            ));
    }
    {
      r.draw(args, d2.color,
            gfx::triangle_factory::make({random_mode(), *state.MODELS[5]}, gfx::color_t{}, rc()),
            gfx::triangle_factory::make({random_mode(), *state.MODELS[6]}, gfx::color_t{}, rc())
            );
    }
    {
      std::vector<gfx::triangle<gfx::vertex_color_attributes>> vec;
      vec.emplace_back(gfx::triangle_factory::make({random_mode(), *state.MODELS[7]}, gfx::color_t{}, rc()));
      vec.emplace_back(gfx::triangle_factory::make({random_mode(), *state.MODELS[8]}, gfx::color_t{}, rc()));

      r.draw(args, d2.color, std::move(vec));
    }

    auto triangle_color = sf.make_triangle({gfx::draw_mode::TRIANGLES, *state.MODELS[9]}, gfx::color_t{},
        gfx::LIST_OF_COLORS::PINK);

    auto triangle_list_colors = sf.make_triangle({gfx::draw_mode::TRIANGLES, *state.MODELS[10]},
        gfx::color_t{}, multicolor_triangle);

    auto triangle_texture = sf.make_triangle({gfx::draw_mode::TRIANGLES, *state.MODELS[11]},
        gfx::uv_t{});

    auto triangle_wireframe = sf.make_triangle({gfx::draw_mode::LINE_LOOP, *state.MODELS[12]},
        gfx::wireframe_t{});

        */
    auto cube_texture = sf.make_textured_cube({gfx::draw_mode::TRIANGLE_STRIP, *state.MODELS[13],
        {0.15f, 0.15f, 0.15f}}, gfx::uv_t{});

    auto cube_color = sf.make_spotted_cube({gfx::draw_mode::TRIANGLE_STRIP, *state.MODELS[14],
        {0.25f, 0.25f, 0.25f}}, gfx::color_t{}, ::gfx::LIST_OF_COLORS::BLUE);

    auto cube_wf = sf.make_wireframe_cube({gfx::draw_mode::LINE_LOOP, *state.MODELS[15],
        {0.25f, 0.25f, 0.25f}}, gfx::wireframe_t{});

    /*
    auto rectangle_color = sf.make_rectangle({gfx::draw_mode::TRIANGLE_STRIP, *state.MODELS[16]},
        gfx::color_t{}, ::gfx::LIST_OF_COLORS::YELLOW);

    auto rectangle_list_colors = sf.make_rectangle({gfx::draw_mode::TRIANGLE_STRIP, *state.MODELS[17],
        height, width}, gfx::color_t{}, multicolor_rect);

    auto rectangle_texture = sf.make_rectangle({gfx::draw_mode::TRIANGLE_STRIP, *state.MODELS[18], height,
        width}, gfx::uv_t{});

    auto rectangle_wireframe = sf.make_rectangle({gfx::draw_mode::LINE_LOOP, *state.MODELS[19], height,
        width}, gfx::wireframe_t{});

    auto polygon_color = sf.make_polygon({gfx::draw_mode::TRIANGLE_FAN, *state.MODELS[20], 5},
        gfx::color_t{}, gfx::LIST_OF_COLORS::DARK_ORANGE);

    auto polygon_texture = sf.make_polygon({gfx::draw_mode::TRIANGLE_FAN, *state.MODELS[21], 7},
        gfx::uv_t{});

    auto polygon_wireframe = sf.make_polygon({gfx::draw_mode::LINE_LOOP, *state.MODELS[22], 7},
        gfx::wireframe_t{});

    //auto polygon_list_of_color = sf.make_polygon(gfx::draw_mode::TRIANGLE_FAN, *zp1, 5,
    //multicolor_triangle);

    r.draw(args, d2.color, triangle_color, triangle_list_colors, std::move(polygon_color),
                                           rectangle_color, rectangle_list_colors
                                           // polygon_list_of_color,
                                           );

    r.draw(args, d2.texture_wall, triangle_texture, rectangle_texture);//, polygon_texture);
    r.draw(args, d2.texture_container, std::move(polygon_texture));
    r.draw(args, d2.wireframe, std::move(polygon_wireframe), triangle_wireframe, rectangle_wireframe);

    */
    r.draw(args, d3.color, cube_color);
    r.draw(args, d3.texture, cube_texture);
    r.draw(args, d3.wireframe, cube_wf);
    rd.end();
  }
};

} // ns game::boomhs
