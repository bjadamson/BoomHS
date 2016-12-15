#pragma once
#include <engine/gfx/opengl/glew.hpp>
#include <engine/gfx/opengl/global.hpp>
#include <stlw/format.hpp>
#include <stlw/tuple.hpp>
#include <SOIL.h>
#include <boost/filesystem/path.hpp>
#include <array>

namespace engine::gfx::opengl
{

struct texture_info {
  GLenum mode;
  GLuint id;
};

namespace impl
{

template<typename L>
void
load_image(L &logger, char const* path, GLenum const target)
{
  int w = 0, h = 0;
  unsigned char *pimage = SOIL_load_image(path, &w, &h, 0, SOIL_LOAD_RGB);
  if (nullptr == pimage) {
    auto const fmt =
        fmt::sprintf("image at path '%s' failed to load, reason '%s'", path, SOIL_last_result());
    logger.error(fmt);
    std::abort();
  }
  ON_SCOPE_EXIT([&]() { SOIL_free_image_data(pimage); });
  glTexImage2D(target, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pimage);
  glGenerateMipmap(target);
}

} // ns impl

namespace fs = boost::filesystem;

template<typename L, typename ...Paths>
static auto
load_3d_texture(L &logger, Paths const&... paths)
{
  GLenum constexpr TEXTURE_MODE = GL_TEXTURE_CUBE_MAP;

  // Here is our guarantee, allowing us to have the convenient interface.
  auto constexpr N = 6;
  static_assert(N == sizeof...(paths));

  std::array<char const*, N> const arr{{ paths... }};
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

  stlw::zip(arr.begin(), arr.end(), directions.begin(), [&logger](auto const& path, auto const& target) {
      impl::load_image(logger, path, target);
  });

  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  return t;
}

template <typename L>
static auto
load_2d_texture(L &logger, boost::filesystem::path const& path)
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

  impl::load_image(logger, path.c_str(), TEXTURE_MODE);
  return t;
}

} // ns engine::gfx::opengl
