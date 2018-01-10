#pragma once
#include <memory>

#include <SOIL.h>

#include <opengl/glew.hpp>
#include <opengl/global.hpp>

#include <stlw/format.hpp>
#include <stlw/tuple.hpp>

namespace opengl
{

struct texture_info {
  GLenum mode;
  GLuint id;
};

namespace impl
{

using pimage_t = std::unique_ptr<unsigned char, void (*)(unsigned char*)>;

struct image_data_t
{
  int width, height;
  pimage_t data;
};

template<typename L>
auto
load_image_into_memory(L &logger, char const* path)
{
  int w = 0, h = 0;
  unsigned char *pimage = SOIL_load_image(path, &w, &h, 0, SOIL_LOAD_RGB);
  if (nullptr == pimage) {
    auto const fmt =
        fmt::sprintf("image at path '%s' failed to load, reason '%s'", path, SOIL_last_result());
    LOG_ERROR(fmt);
    std::abort();
  }
  pimage_t image_data{pimage, &SOIL_free_image_data};
  return image_data_t{w, h, MOVE(image_data)};
}

template<typename L>
void
upload_image(L &logger, std::string const& filename, GLenum const target)
{
  std::string const path = "assets/" + filename;
  auto const image_data = load_image_into_memory(logger, path.c_str());

  auto const width = image_data.width;
  auto const height = image_data.height;
  auto const* data = image_data.data.get();

  glTexImage2D(target, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
}

} // ns impl

namespace texture
{

template <typename L>
static auto
allocate_texture(L &logger, std::string const& filename)
{
  GLenum constexpr TEXTURE_MODE = GL_TEXTURE_2D;

  GLuint texture_id;
  glGenTextures(1, &texture_id);
  texture_info const t{TEXTURE_MODE, texture_id};

  global::texture_bind(t);
  ON_SCOPE_EXIT([&t]() { global::texture_unbind(t); });

  // Set texture wrapping to GL_REPEAT (usually basic wrapping method)
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Set texture filtering parameters
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  impl::upload_image(logger, filename, TEXTURE_MODE);
  return t;
}

template<typename L>
static auto
upload_3dcube_texture(L &logger, std::vector<std::string> const& paths)
{
  assert(paths.size() == 6);
  GLenum constexpr TEXTURE_MODE = GL_TEXTURE_CUBE_MAP;

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
  LOG_ANY_GL_ERRORS(logger, "glGenTextures");

  global::texture_bind(t);
  ON_SCOPE_EXIT([&t]() { global::texture_unbind(t); });
  LOG_ANY_GL_ERRORS(logger, "texture_bind");

  auto const upload_fn = [&logger](std::string const& filename, auto const& target) {
      impl::upload_image(logger, filename, target);
  };
  auto const paths_tuple = std::make_tuple(paths[0], paths[1], paths[2], paths[3], paths[4], paths[5]);
  stlw::zip(upload_fn, directions.begin(), paths_tuple);

  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  glGenerateMipmap(TEXTURE_MODE);
  LOG_ANY_GL_ERRORS(logger, "glGenerateMipmap");

  return t;
}

} // ns texture
} // ns opengl
