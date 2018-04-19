#include <opengl/texture.hpp>

#include <opengl/global.hpp>
#include <gfx/gl_sdl_log.hpp>

#include <stlw/algorithm.hpp>
#include <stlw/tuple.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/glew.hpp>
#include <extlibs/soil.hpp>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

namespace
{

using namespace opengl;

Result<ImageData, std::string>
upload_image(stlw::Logger &logger, std::string const& filename, GLenum const target,
    GLint const format)
{
  std::string const path = "assets/" + filename;
  auto image_data = TRY_MOVEOUT(texture::load_image(logger, path.c_str(), format));

  auto const width = image_data.width;
  auto const height = image_data.height;
  auto const* data = image_data.data.get();

  LOG_TRACE_SPRINTF("uploading %s with w: %i, h: %i", path, width, height);
  glTexImage2D(target, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

  return OK_MOVE(image_data);
}

} // ns anonymous

namespace opengl
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// TextureInfo
void
TextureInfo::destroy()
{
  glDeleteTextures(TextureInfo::NUM_BUFFERS, &id);
}

std::string
TextureInfo::to_string() const
{
  return fmt::sprintf("id: %u, mode: %i, (w, h) : (%i, %i), uv_max: %i",
      id, mode, width, height, uv_max);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// FBInfo
std::string
FBInfo::to_string() const
{
  return fmt::sprintf("id: %u, color_buffer: %u", id, color_buffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RBInfo
std::string
RBInfo::to_string() const
{
  return fmt::sprintf("depth: %u", depth);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TextureTable
void
TextureTable::add_texture(TextureFilenames &&tf, Texture &&ta)
{
  auto pair = std::make_pair(MOVE(tf), MOVE(ta));
  data_.emplace_back(MOVE(pair));
}

std::optional<TextureInfo>
TextureTable::find(std::string const& name) const
{
  auto const cmp = [&name](auto const& it)
  {
    FOR(i, it.first.filenames.size()) {
      auto const& fn = it.first.filenames[i];
    }
    return it.first.name == name;
  };
  auto const it = std::find_if(data_.cbegin(), data_.cend(), cmp);
  return it == data_.cend() ? std::nullopt : std::make_optional(it->second.resource());
}

} // ns opengl

namespace opengl::texture
{

ImageResult
load_image(stlw::Logger &logger, char const* path, GLint const format)
{
  int w = 0, h = 0;

  int const soil_format = format == GL_RGBA ? SOIL_LOAD_RGBA : SOIL_LOAD_RGB;
  unsigned char *pimage = SOIL_load_image(path, &w, &h, 0, soil_format);
  if (nullptr == pimage) {
    auto const fmt = fmt::sprintf("image at path '%s' failed to load, reason '%s'", path,
        SOIL_last_result());
    return Err(fmt);
  }
  pimage_t image_data{pimage, &SOIL_free_image_data};
  return Ok(ImageData{w, h, MOVE(image_data)});
}

GLint
wrap_mode_from_string(char const* name)
{
  auto const cmp = [&name](char const* str) {
    auto const len = std::strlen(str);
    return std::strncmp(name, str, len) == 0;
  };
  if (cmp("clamp")) {
    return GL_CLAMP_TO_EDGE;
  }
  else if (cmp("repeat")) {
    return GL_REPEAT;
  }
  else if (cmp("mirror_repeat")) {
    return GL_MIRRORED_REPEAT;
  }

  // Invalid
  std::abort();
}

TextureResult
allocate_texture(stlw::Logger &logger, std::string const& filename, GLint const format,
    GLint const wrap, GLint const uv_max)
{
  assert(ANYOF(format == GL_RGB, format == GL_RGBA));

  GLenum constexpr TEXTURE_MODE = GL_TEXTURE_2D;

  TextureInfo ti;
  ti.mode = TEXTURE_MODE;
  glGenTextures(1, &ti.id);
  LOG_TRACE_SPRINTF("allocating texture %s TextureID %u", filename, ti.id);

  global::texture_bind(ti);
  ON_SCOPE_EXIT([&ti]() { global::texture_unbind(ti); });

  // Set texture wrapping mode
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_S, wrap);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_T, wrap);

  // Set texture filtering parameters
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  auto const image_data = TRY_MOVEOUT(upload_image(logger, filename, TEXTURE_MODE, format));
  ti.height = image_data.height;
  ti.width = image_data.width;

  ti.uv_max = uv_max;
  return Ok(Texture{MOVE(ti)});
}

TextureResult
upload_3dcube_texture(stlw::Logger &logger, std::vector<std::string> const& paths,
    GLint const format)
{
  assert(paths.size() == 6);
  assert(ANYOF(format == GL_RGB, format == GL_RGBA));
  GLenum constexpr TEXTURE_MODE = GL_TEXTURE_CUBE_MAP;

  static constexpr auto directions = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // back
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, // right
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // front
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // left
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // top
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // bottom
  };

  TextureInfo ti;
  ti.mode = TEXTURE_MODE;
  glGenTextures(1, &ti.id);

  LOG_ANY_GL_ERRORS(logger, "glGenTextures");

  global::texture_bind(ti);
  ON_SCOPE_EXIT([&ti]() { global::texture_unbind(ti); });
  LOG_ANY_GL_ERRORS(logger, "texture_bind");

  auto const upload_fn = [&format, &logger, &ti](std::string const& filename, auto const& target)
    -> Result<stlw::none_t, std::string>
  {
    auto const image_data = TRY_MOVEOUT(upload_image(logger, filename, target, format));

    // Either the height is unset (0) or all height/width are the same.
    assert(ti.height == 0 || ti.height == image_data.height);
    assert(ti.width == 0 || ti.width == image_data.width);

    ti.height = image_data.height;
    ti.width = image_data.width;

    return OK_NONE;
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

  return Ok(Texture{MOVE(ti)});
}

} // ns opengl::texture
