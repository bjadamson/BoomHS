#pragma once
#include <gfx/opengl/glew.hpp>
#include <gfx/opengl/global.hpp>
#include <gfx/opengl/image_data.hpp>
#include <stlw/format.hpp>
#include <stlw/tuple.hpp>

namespace gfx::opengl
{

struct texture_info {
  GLenum mode;
  GLuint id;
};

namespace impl
{

template<typename L>
void
upload_image(L &logger, image_data_t const& image, GLenum const target)
{
  glTexImage2D(target, 0, GL_RGB, image.width, image.height, 0, GL_RGB, GL_UNSIGNED_BYTE,
      image.data.get());
  glGenerateMipmap(target);
}

} // ns impl

template<typename L, typename ...ImageData>
static auto
upload_3dcube_texture(L &logger, std::tuple<ImageData...> const& image_data)
{
  GLenum constexpr TEXTURE_MODE = GL_TEXTURE_CUBE_MAP;

  // Here is our guarantee, allowing us to have the convenient interface.
  auto constexpr N = 6;
  static_assert(N == sizeof...(ImageData), "Uploading a 3D cube requires 6 images.");

  static constexpr auto directions = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // back
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, // right
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // front
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // left
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // top
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // bottom
  };

  GLuint texture_id;
  glGenTextures(1, &texture_id);

  texture_info const t{TEXTURE_MODE, texture_id};

  global::texture_bind(t);
  ON_SCOPE_EXIT([&t]() { global::texture_unbind(t); });

  auto const fn = [&logger](auto const& image_data, auto const& target) {
      impl::upload_image(logger, image_data, target);
  };
  stlw::zip(fn, directions.begin(), image_data);

  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  return t;
}

template <typename L>
static auto
upload_2d_texture(L &logger, image_data_t const& image_data)
{
  GLenum constexpr TEXTURE_MODE = GL_TEXTURE_2D;

  GLuint texture_id;
  glGenTextures(1, &texture_id);
  texture_info const t{TEXTURE_MODE, texture_id};

  global::texture_bind(t);
  ON_SCOPE_EXIT([&t]() { global::texture_unbind(t); });

  // Set texture wrapping to GL_REPEAT (usually basic wrapping method)
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Set texture filtering parameters
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  impl::upload_image(logger, image_data, TEXTURE_MODE);
  return t;
}

} // ns gfx::opengl
