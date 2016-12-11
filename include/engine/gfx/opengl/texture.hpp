#pragma once
#include <SOIL.h>
#include <engine/gfx/opengl/glew.hpp>
#include <engine/gfx/opengl/global.hpp>
#include <stlw/format.hpp>

namespace engine::gfx::opengl
{

struct texture_info
{
  GLenum mode;
  GLuint id;
};

template<typename L>
static auto
load_3d_texture(L &logger, char const *path)
{
  GLenum constexpr TEXTURE_MODE = GL_TEXTURE_CUBE_MAP;

  GLuint texture_id;
  glGenTextures(1, &texture_id);

  texture_info const t{TEXTURE_MODE, texture_id};

  global::texture_bind(t);
  ON_SCOPE_EXIT([&t]() { global::texture_unbind(t); });

  auto const load_image = [&path, &logger](GLenum const target) {
    int w = 0, h = 0;
    unsigned char *pimage = SOIL_load_image(path, &w, &h, 0, SOIL_LOAD_RGB);
    if (nullptr == pimage) {
      auto const fmt = fmt::sprintf("image at path '%s' failed to load, reason '%s'", path,
          SOIL_last_result());
      logger.error(fmt);
      std::abort();
    }
    ON_SCOPE_EXIT([&]() { SOIL_free_image_data(pimage); });
    glTexImage2D(target, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pimage);
    //glGenerateMipmap(mode); // TODO: valid?
  };

  load_image(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
  load_image(GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
  load_image(GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
  load_image(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
  load_image(GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
  load_image(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);

  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  return t;
}

template<typename L>
static auto
load_2d_texture(L &logger, char const *path)
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

  int w = 0, h = 0;
  unsigned char *pimage = SOIL_load_image(path, &w, &h, 0, SOIL_LOAD_RGB);
  if (nullptr == pimage) {
    auto const fmt = fmt::sprintf("image at path '%s' failed to load, reason '%s'", path,
        SOIL_last_result());
    logger.error(fmt);
    std::abort();
  }
  ON_SCOPE_EXIT([&]() { SOIL_free_image_data(pimage); });

  glTexImage2D(TEXTURE_MODE, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pimage);
  glGenerateMipmap(TEXTURE_MODE);

  return t;
}

} // ns engine::gfx::opengl
