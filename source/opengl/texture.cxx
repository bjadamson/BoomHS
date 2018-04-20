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
upload_image(stlw::Logger &logger, std::string const& path, GLenum const target,
    GLint const format)
{
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
TextureInfo::TextureInfo()
  : mode(0)
  , id(0)
  , bound(false)
{
}

void
TextureInfo::bind() const
{
  FOR_DEBUG_ONLY([&]() { assert(bound == false); });

  global::texture_bind(*this);
  FOR_DEBUG_ONLY([&]() { bound = true; });
}

void
TextureInfo::unbind() const
{
  FOR_DEBUG_ONLY([&]() { assert(bound == true); });

  global::texture_unbind(*this);
  FOR_DEBUG_ONLY([&]() { bound = false; });
}

void
TextureInfo::destroy()
{
  glDeleteTextures(TextureInfo::NUM_BUFFERS, &id);
}

GLint
TextureInfo::get_fieldi(GLenum const name)
{
  FOR_DEBUG_ONLY([&]() { assert(bound == true); });

  GLint value;
  glGetTexParameteriv(this->mode, name, &value);
  return value;
}

void
TextureInfo::set_fieldi(GLenum const name, GLint const value)
{
  FOR_DEBUG_ONLY([&]() { if (!this->bound) { std::abort(); }});

  glTexParameteri(this->mode, name, value);
}

std::string
TextureInfo::to_string() const
{
  return fmt::sprintf("id: %u, mode: %i, (w, h) : (%i, %i), uv_max: %f",
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

#define FIND_TF(name)                                                                              \
  [&]() {                                                                                          \
    auto const cmp = [&name](auto const& it)                                                       \
    {                                                                                              \
      FOR(i, it.first.filenames.size()) {                                                          \
        auto const& fn = it.first.filenames[i];                                                    \
      }                                                                                            \
      return it.first.name == name;                                                                \
    };                                                                                             \
    return std::find_if(data_.begin(), data_.end(), cmp);                                          \
  }()

TextureFilenames const*
TextureTable::lookup_nickname(std::string const& name) const
{
  auto const it = FIND_TF(name);
  return it == data_.cend() ? nullptr : &it->first;
}

TextureInfo*
TextureTable::find(std::string const& name)
{
  auto it = FIND_TF(name);
  return it == data_.end() ? nullptr : &it->second.resource();
}

TextureInfo const*
TextureTable::find(std::string const& name) const
{
  auto const it = FIND_TF(name);
  return it == data_.cend() ? nullptr : &it->second.resource();
}

#undef FIND_TF

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
    GLint const uv_max)
{
  assert(ANYOF(format == GL_RGB, format == GL_RGBA));

  TextureInfo ti;
  ti.mode = GL_TEXTURE_2D;
  glGenTextures(1, &ti.id);
  LOG_TRACE_SPRINTF("allocating texture %s TextureID %u", filename, ti.id);

  // This next bit comes from tracking down a weird bug. Without this extra scope, the texture info
  // does not get unbound because the move constructor for the AutoResource(Texture) moves the
  // TextureInfo instance inside the TextureResult. This zeroes out the id value inside the local
  // variable "ti", where at this point the ON_SCOP_EXIT lambda executes using this moved-from
  // instance of "ti".
  //
  // Using the explicit scope guarantees the texture is unbound when the stack frame unwinds.
  //
  // note: Using while_bound() doesn't work here because TRY_MOVEOUT returns a value.
  {
    ti.bind();
    ON_SCOPE_EXIT([&ti]() { ti.unbind(); });

    auto const image_data = TRY_MOVEOUT(upload_image(logger, filename, ti.mode, format));
    ti.height = image_data.height;
    ti.width = image_data.width;

    ti.uv_max = uv_max;

    glGenerateMipmap(ti.mode);
    LOG_ANY_GL_ERRORS(logger, "glGenerateMipmap");
  }

  return Ok(Texture{MOVE(ti)});
}

TextureResult
upload_3dcube_texture(stlw::Logger &logger, std::vector<std::string> const& paths,
    GLint const format)
{
  assert(paths.size() == 6);
  assert(ANYOF(format == GL_RGB, format == GL_RGBA));

  static constexpr auto directions = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // back
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, // right
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // front
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // left
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // top
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // bottom
  };

  TextureInfo ti;
  ti.mode = GL_TEXTURE_CUBE_MAP;
  glGenTextures(1, &ti.id);
  LOG_ANY_GL_ERRORS(logger, "glGenTextures");

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

  auto const fn = [&]() {
    stlw::zip(upload_fn, directions.begin(), paths_tuple);

    LOG_ANY_GL_ERRORS(logger, "glGenerateMipmap");
    glGenerateMipmap(ti.mode);
  };
  ti.while_bound(fn);

  return Ok(Texture{MOVE(ti)});
}

} // ns opengl::texture
