#pragma once
#include <engine/gfx/opengl/context.hpp>
#include <engine/gfx/opengl/program.hpp>
#include <engine/gfx/opengl/scene_renderer.hpp>
#include <engine/gfx/opengl_glew.hpp>
#include <engine/gfx/opengl/vertex_attrib.hpp>
#include <engine/window/sdl_window.hpp>
#include <game/shape.hpp>
#include <stlw/sized_buffer.hpp>
#include <stlw/type_ctors.hpp>
#include <glm/glm.hpp>

namespace engine::gfx::opengl
{

namespace impl {

namespace gl = engine::gfx::opengl;

stlw::result<gl::program, std::string>
load_program(vertex_shader_filename const v, fragment_shader_filename const f)
{
  auto expected_program_id = gl::program_loader::load(v.filename, f.filename);
  if (!expected_program_id) {
    return stlw::make_error(expected_program_id.error());
  }
  return expected_program_id;
}

} // ns impl

auto log_error = [](auto const line) {
  GLenum err = GL_NO_ERROR;
  while ((err = glGetError()) != GL_NO_ERROR) {
    std::cerr << "GL error! '" << std::hex << err << "' line #" << std::to_string(line)
              << std::endl;
    return;
  }
  std::string error = SDL_GetError();
  if (error != "") {
    std::cout << "SLD Error : " << error << std::endl;
    SDL_ClearError();
  }
};

template<typename L>
struct render_args
{
  L &logger;
  glm::mat4 const& view;
  glm::mat4 const& projection;

  render_args(L &l, glm::mat4 const& v, glm::mat4 const& p)
    : logger(l)
    , view(v)
    , projection(p)
  {
  }
};

// For now we assume 10 attributes per-vertex:
// 4 vertex (x,y,z,w)
// 4 colors (r,g,b,a),
// 2 tex coords (u, v)
template<template<class, std::size_t> typename T, typename F, std::size_t V,
  std::size_t NUM_OF_F_PER_VERTEX=10>
class static_shape_template
{
  GLenum const mode_;
  T<F, V> const data_;
public:
  explicit constexpr static_shape_template(GLenum const m, T<F, V> const &d)
    : mode_(m)
    , data_(d)
  {
  }
  constexpr auto draw_mode() const { return this->mode_; }
  constexpr auto data() const { return this->data_.data(); }
  constexpr auto size_in_bytes() const { return V * sizeof(F); }
  constexpr auto vertice_count() const { return V / NUM_OF_F_PER_VERTEX; }
};

template<typename F, std::size_t V>
using static_shape_array = static_shape_template<std::array, F, V>;

template<template<class, class> typename C, typename T, typename A=std::allocator<T>, std::size_t NUM_OF_F_PER_VERTEX=10>
class runtime_shape_template
{
  GLenum const mode_;
  C<T, A> const data_;

  auto length() const { return this->data_.length(); }
public:
  explicit runtime_shape_template(GLenum const m, C<T, A> const &d)
    : mode_(m)
    , data_(d)
  {
  }
  auto draw_mode() const { return this->mode_; }
  auto data() const { return this->data_.data(); }

  auto size_in_bytes() const { return this->length() * sizeof(T); }
  auto vertice_count() const { return this->length() / NUM_OF_F_PER_VERTEX; }
};

template<typename F>
using runtime_sized_array = runtime_shape_template<stlw::sized_buffer, F>;

// float types
using float_triangle = static_shape_array<float, 30>;
using float_rectangle = static_shape_array<float, 40>;
using float_polygon = runtime_sized_array<float>;

class shape_mapper {
  shape_mapper() = delete;

  static int constexpr calculate_number_floats(int const num_vertices, int const num_colors,
      int const num_uv)
  {
    // nf => number of floats for
    auto const num_vertice_floats = num_vertices * 4; // x, y, z, w
    auto const num_color_floats = num_colors * 4;     // r, g, b, a
    auto const num_uv_floats = num_uv * 2;            // u, v
    return num_vertice_floats + num_color_floats + num_uv_floats;
  }

  static constexpr auto map_to_array_floats(game::triangle const& t)
  {
    auto constexpr NUM_VERTICES = 3;
    auto constexpr NUM_COLORS = 3;
    auto constexpr NUM_UV = 3;
    auto constexpr NUM_FLOATS = shape_mapper::calculate_number_floats(NUM_VERTICES, NUM_COLORS,
        NUM_UV);

    auto const floats =  std::array<float, NUM_FLOATS>{
      t.bottom_left.vertex.x, t.bottom_left.vertex.y, t.bottom_left.vertex.z, t.bottom_left.vertex.w,
      t.bottom_left.color.r,  t.bottom_left.color.g,  t.bottom_left.color.b,  t.bottom_left.color.a,
      t.bottom_left.uv.u,     t.bottom_left.uv.v,

      t.bottom_right.vertex.x, t.bottom_right.vertex.y, t.bottom_right.vertex.z, t.bottom_right.vertex.w,
      t.bottom_right.color.r,  t.bottom_right.color.g,  t.bottom_right.color.b,  t.bottom_right.color.a,
      t.bottom_right.uv.u,     t.bottom_right.uv.v,


      t.top_middle.vertex.x, t.top_middle.vertex.y, t.top_middle.vertex.z, t.top_middle.vertex.w,
      t.top_middle.color.r,  t.top_middle.color.g,  t.top_middle.color.b,  t.top_middle.color.a,
      t.top_middle.uv.u,     t.top_middle.uv.v,
    };
    return float_triangle{GL_TRIANGLES, floats};
  }

  static constexpr auto map_to_array_floats(game::rectangle const& r)
  {
    auto constexpr NUM_VERTICES = 4;
    auto constexpr NUM_COLORS = 4;
    auto constexpr NUM_UV = 4;
    auto constexpr NUM_FLOATS = shape_mapper::calculate_number_floats(NUM_VERTICES, NUM_COLORS,
        NUM_UV);

    auto const floats = std::array<float, NUM_FLOATS>{
      r.bottom_left.vertex.x, r.bottom_left.vertex.y, r.bottom_left.vertex.z, r.bottom_left.vertex.w,
      r.bottom_left.color.r,  r.bottom_left.color.g,  r.bottom_left.color.b,  r.bottom_left.color.a,
      r.bottom_left.uv.u,     r.bottom_left.uv.v,

      r.bottom_right.vertex.x, r.bottom_right.vertex.y, r.bottom_right.vertex.z, r.bottom_right.vertex.w,
      r.bottom_right.color.r,  r.bottom_right.color.g,  r.bottom_right.color.b,  r.bottom_right.color.a,
      r.bottom_right.uv.u,     r.bottom_right.uv.v,

      r.top_right.vertex.x, r.top_right.vertex.y, r.top_right.vertex.z, r.top_right.vertex.w,
      r.top_right.color.r,  r.top_right.color.g,  r.top_right.color.b,  r.top_right.color.a,
      r.top_right.uv.u,     r.top_right.uv.v,

      r.top_left.vertex.x, r.top_left.vertex.y, r.top_left.vertex.z, r.top_left.vertex.w,
      r.top_left.color.r,  r.top_left.color.g,  r.top_left.color.b,  r.top_left.color.a,
      r.top_left.uv.u,     r.top_left.uv.v,
    };
    return float_rectangle{GL_QUADS, floats};
  }

  static auto map_to_array_floats(game::polygon const& p)
  {
    auto const num_vertices = p.num_vertices();
    auto const num_colors = num_vertices; // TODO: bye-bye
    auto const num_uv = num_colors;       // also this assumption too.
    auto const num_floats = shape_mapper::calculate_number_floats(num_vertices, num_colors, num_uv);

    stlw::sized_buffer<float> floats{static_cast<size_t>(num_floats)};
    for (auto i{0}, j{0}; j < floats.length(); ++i) {
      auto &vertice = p.vertices[i];
      floats[j++] = vertice.vertex.x;
      floats[j++] = vertice.vertex.y;
      floats[j++] = vertice.vertex.z;
      floats[j++] = vertice.vertex.w;

      floats[j++] = vertice.color.r;
      floats[j++] = vertice.color.g;
      floats[j++] = vertice.color.b;
      floats[j++] = vertice.color.a;

      floats[j++] = vertice.uv.u;
      floats[j++] = vertice.uv.v;
    }
    // TODO: deprecated
    return float_polygon{GL_POLYGON, floats};
  }

public:
  template<typename ...T, typename ...R>
  static constexpr auto map_to_floats(T &&... shapes)
  {
    return std::make_tuple(shape_mapper::map_to_array_floats(shapes)...);
  }
};

class gfx_engine
{
  using W = ::engine::window::sdl_window;

  W window_;
  opengl_context rc0_, rc1_;

  gfx_engine(W &&w, opengl_context &&r0, opengl_context &&r1)
    : window_(std::move(w))
    , rc0_(std::move(r0))
    , rc1_(std::move(r1))
  {
  }

  NO_COPY(gfx_engine);
  gfx_engine &operator=(gfx_engine &&) = delete;

  friend struct opengl_library;

public:
  // move-assignment OK.
  gfx_engine(gfx_engine &&other)
      : window_(std::move(other.window_))
      , rc0_(std::move(other.rc0_))
      , rc1_(std::move(other.rc1_))
  {
    // background color
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  }

  void begin()
  {
    // Render
    glClear(GL_COLOR_BUFFER_BIT);
  }
  void end()
  {
    // Update window with OpenGL rendering
    SDL_GL_SwapWindow(this->window_.raw());
  }

  template <typename Args, typename ...S>
  void draw0(Args const& args, S const&... shapes)
  {
    auto const gl_mapped_shapes = shape_mapper::map_to_floats(shapes...);
    renderer::draw_scene(args.logger, this->rc0_, args.view, args.projection, gl_mapped_shapes);
  }

  template <typename Args, typename ...S>
  void draw1(Args const& args, S const&... shapes)
  {
    auto const gl_mapped_shapes = shape_mapper::map_to_floats(shapes...);

    renderer::draw_scene(args.logger, this->rc1_, args.view, args.projection, gl_mapped_shapes);
  }
};

struct opengl_library {
  opengl_library() = delete;

  static void init()
  {
    // for now, to simplify rendering
    glEnable(GL_DEPTH_TEST);
    // glDisable(GL_CULL_FACE);
  }

  static inline void destroy() {}

  template <typename L, typename W>
  static inline stlw::result<gfx_engine, std::string> make_gfx_engine(L &logger, W &&window)
  {
    auto const make_ctx = [&logger](auto &&phandle, char const* asset_path) {
      auto context = context_factory::make_opengl_context(std::move(phandle), asset_path);
      auto vertex_attribute_config = make_vertex_attribute_config(context);
      global::set_vertex_attributes(logger, vertex_attribute_config);
      return context;
    };

    // TODO: can they share the same program???
    DO_MONAD(auto phandle0, impl::load_program("shader.vert", "shader.frag"));
    auto rc0 = make_ctx(std::move(phandle0), "assets/wall.jpg");

    DO_MONAD(auto phandle1, impl::load_program("shader.vert", "shader.frag"));
    auto rc1 = make_ctx(std::move(phandle1), "assets/container.jpg");

    return gfx_engine{std::move(window), std::move(rc0), std::move(rc1)};
  }
};

} // ns engine::gfx::opengl
