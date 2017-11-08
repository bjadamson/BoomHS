#pragma once
#include <opengl/colors.hpp>
#include <opengl/global.hpp>
#include <opengl/shader_program.hpp>
#include <opengl/texture.hpp>

#include <backward/backward.hpp>

namespace opengl
{

class opengl_vao {
  GLuint vao_ = 0;

  static constexpr auto NUM_BUFFERS = 1;

public:
  explicit opengl_vao()
  {
    glGenVertexArrays(NUM_BUFFERS, &this->vao_);
  }

  NO_COPY(opengl_vao);
  NO_MOVE_ASSIGN(opengl_vao);

  ~opengl_vao()
  {
    glDeleteVertexArrays(NUM_BUFFERS, &this->vao_);
  }

  // move-construction OK.
  opengl_vao(opengl_vao &&other)
      : vao_(other.vao_)
  {
    other.vao_ = 0;
  }

  inline auto gl_raw_value() const { return this->vao_; }
};

class color2d_context
{
  opengl_vao vao_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(color2d_context);
  color2d_context() = default;

  static bool constexpr IS_2D = true;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  auto const& vao() const { return this->vao_; }
};

class color3d_context
{
  opengl_vao vao_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(color3d_context);
  color3d_context() = default;

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  auto const& vao() const { return this->vao_; }
};

class wall_context
{
  opengl_vao vao_;
  static constexpr GLuint INSTANCE_COUNT = 3u;

public:
  MOVE_CONSTRUCTIBLE_ONLY(wall_context);
  wall_context() = default;

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = true;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = false;

  inline auto instance_count() const { return wall_context::INSTANCE_COUNT; }
  inline auto const& vao() const { return this->vao_; }
};

class texture3d_context
{
  opengl_vao vao_;
  texture_info texture_info_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(texture3d_context);

  explicit texture3d_context(texture_info const t)
      : texture_info_(t)
  {
  }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  auto const& vao() const { return this->vao_; }
  auto texture() const { return this->texture_info_; }
};

class texture_3dcube_context
{
  opengl_vao vao_;
  texture_info texture_info_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(texture_3dcube_context);

  explicit texture_3dcube_context(texture_info const t)
      : texture_info_(t)
  {
  }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  auto const& vao() const { return this->vao_; }
  auto texture() const { return this->texture_info_; }
};

class skybox_context
{
  opengl_vao vao_;
  texture_info texture_info_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(skybox_context);

  explicit skybox_context(texture_info const t)
      : texture_info_(t)
  {
  }

  static bool constexpr IS_2D = false;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = true;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  auto const& vao() const { return this->vao_; }
  auto texture() const { return this->texture_info_; }
};

class texture2d_context
{
  opengl_vao vao_;
  texture_info texture_info_;

public:
  explicit texture2d_context(texture_info const t)
      : texture_info_(t)
  {
  }

  MOVE_CONSTRUCTIBLE_ONLY(texture2d_context);

  static bool constexpr IS_2D = true;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = false;
  static bool constexpr HAS_TEXTURE = true;

  auto const& vao() const { return this->vao_; }
  auto texture() const { return this->texture_info_; }
};

template<bool IS_2D_T>
class wireframe_context
{
  opengl_vao vao_;
  std::array<float, 4> color_;

public:
  MOVE_CONSTRUCTIBLE_ONLY(wireframe_context);

  explicit wireframe_context(std::array<float, 4> const &color)
      : color_(color)
  {
  }

  static bool constexpr IS_2D = IS_2D_T;
  static bool constexpr IS_INSTANCED = false;
  static bool constexpr IS_SKYBOX = false;
  static bool constexpr HAS_COLOR_UNIFORM = true;
  static bool constexpr HAS_TEXTURE = false;

  inline auto color() const { return this->color_; }
  auto const& vao() const { return this->vao_; }
};

using wireframe2d_context = wireframe_context<true>;
using wireframe3d_context = wireframe_context<false>;

struct opengl_context2d
{
  template <typename L>
  auto static make_wireframe2d(L &logger, std::array<float, 3> const &c)
  {
    constexpr auto ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};
    return wireframe2d_context{color};
  }

  template<typename L>
  static auto make_2dtexture(L &logger, IMAGES const image)
  {
    return texture2d_context(texture::allocate_texture(logger, image));
  }
};

struct opengl_context3d
{
  template <typename L, typename ...IMAGES>
  auto static make_texture3dcube(L &logger, IMAGES const&... images)
  {
    auto const tid = texture::upload_3dcube_texture(logger, images...);
    return texture_3dcube_context{tid};
  }

  template <typename L, typename ...IMAGES>
  auto static make_skybox(L &logger, IMAGES const&... images)
  {
    auto const tid = texture::upload_3dcube_texture(logger, images...);
    return skybox_context{tid};
  }

  template <typename L>
  auto static make_wireframe3d(L &logger, std::array<float, 3> const &c)
  {
    constexpr auto ALPHA = 1.0f;
    std::array<float, 4> const color{c[0], c[1], c[2], ALPHA};
    return wireframe3d_context{color};
  }
};

struct opengl_contexts
{
  opengl_context2d d2;
  opengl_context3d d3;

  MOVE_CONSTRUCTIBLE_ONLY(opengl_contexts);

  template<typename L>
  opengl_contexts(L &logger)
    : d2(logger)
    , d3(logger)
  {
  }
};

} // ns opengl
