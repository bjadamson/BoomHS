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
#include <sstream>
#include <vector>
#include <utility>

#define DEBUG_ASSERT_BOUND()                                                                       \
  FOR_DEBUG_ONLY([&]() { assert(this->bound == true); });

#define DEBUG_ASSERT_NOT_BOUND()                                                                   \
  FOR_DEBUG_ONLY([&]() { assert(this->bound == false); });

#define DEBUG_BIND()                                                                               \
  FOR_DEBUG_ONLY([&]() { this->bound = true; });

#define DEBUG_UNBIND()                                                                             \
  FOR_DEBUG_ONLY([&]() { this->bound = false; });

using namespace boomhs;
using namespace opengl;

namespace
{

struct GpuUploadConfig
{
  GLenum const format;
  GLenum const target;
  GLenum const texture_unit;
};

Result<ImageData, std::string>
upload_image_gpu(stlw::Logger &logger, std::string const& path, GpuUploadConfig const& guc)
{
  auto image_data = TRY_MOVEOUT(texture::load_image(logger, path.c_str(), guc.format));

  auto const width = image_data.width;
  auto const height = image_data.height;
  auto const* data = image_data.data.get();

  LOG_TRACE_SPRINTF("uploading %s with w: %i, h: %i", path, width, height);

  auto const format       = guc.format;
  auto const target       = guc.target;
  auto const texture_unit = guc.texture_unit;

  glActiveTexture(texture_unit);
  glTexImage2D(target, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

  return OK_MOVE(image_data);
}

} // ns anon

namespace opengl
{
////////////////////////////////////////////////////////////////////////////////////////////////////
// IdTextureUnit
IdTextureUnit::IdTextureUnit()
  : id(0)
  , texture_unit(GL_TEXTURE0)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IdTextureUnits
IdTextureUnits::IdTextureUnits(size_t const num)
  : num_active_(num)
{
  assert(num < DEBUG_HACK_MAX_NUM_TEXTURE_UNITS);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TextureInfo
TextureInfo::TextureInfo()
  : target(GL_TEXTURE_2D)
  , ids_units(1)
  , bound(false)
{
}

void
TextureInfo::gen_texture(stlw::Logger &logger, GLsizei const num)
{
  glGenTextures(num, &id());
  LOG_ANY_GL_ERRORS(logger, "glGenTextures");
}

void
TextureInfo::bind(stlw::Logger& logger)
{
  DEBUG_ASSERT_NOT_BOUND();

  FOR(i, ids_units.size()) {
    glActiveTexture(GL_TEXTURE0 + i);
    global::texture_bind(*this);
  }
  DEBUG_BIND();
}

void
TextureInfo::unbind(stlw::Logger& logger)
{
  DEBUG_ASSERT_BOUND();

  // This is an expected no-op operation.
  //
  // Make sure this isn't masking any problems, try commenting out and see if rendering changes.
  glActiveTexture(GL_TEXTURE0);

  global::texture_unbind(*this);
  DEBUG_UNBIND();
}

void
TextureInfo::destroy()
{
  DEBUG_ASSERT_NOT_BOUND();

  glDeleteTextures(TextureInfo::NUM_BUFFERS, &id());
}

GLint
TextureInfo::get_fieldi(GLenum const name)
{
  DEBUG_ASSERT_BOUND();

  GLint value;
  glGetTexParameteriv(this->target, name, &value);
  return value;
}

void
TextureInfo::set_fieldi(GLenum const name, GLint const value)
{
  DEBUG_ASSERT_BOUND();

  glTexParameteri(this->target, name, value);
}

std::string
TextureInfo::to_string() const
{
  return fmt::sprintf("(TextureInfo) id: %u, target: %i, (w, h) : (%i, %i), uv_max: %f",
      id(), target, width, height, uv_max);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// FBInfo
FBInfo::FBInfo(Dimensions const& d, ScreenSize const& ss)
  : dimensions(d)
  , screen_size(ss)
{
  glGenFramebuffers(1, &id);
}

void
FBInfo::bind(stlw::Logger& logger)
{
  DEBUG_ASSERT_NOT_BOUND();

  glBindFramebuffer(GL_FRAMEBUFFER, id);
  glViewport(dimensions.x, dimensions.y, dimensions.w, dimensions.h);

  DEBUG_BIND();
}

void
FBInfo::unbind(stlw::Logger& logger)
{
  DEBUG_ASSERT_BOUND();

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, screen_size.width, screen_size.height);

  DEBUG_UNBIND();
}

void
FBInfo::destroy()
{
  DEBUG_ASSERT_NOT_BOUND();

  glDeleteFramebuffers(1, &id);
}

std::string
FBInfo::to_string() const
{
  return fmt::sprintf("(FBInfo) id: %u", id);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// RBInfo
std::string
RBInfo::to_string() const
{
  return fmt::sprintf("(RBInfo) depth: %u", depth);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TextureTable
void
TextureTable::add_texture(TextureFilenames &&tf, Texture &&ta)
{
  auto pair = std::make_pair(MOVE(tf), MOVE(ta));
  data_.emplace(MOVE(pair));
}

std::string
TextureTable::list_of_all_names(char const delim) const
{
  std::stringstream buffer;
  for (auto const& it : *this) {
    buffer << it.first.name;
    buffer << delim;
  }
  return buffer.str();
}

std::optional<size_t>
TextureTable::index_of_nickname(std::string const& name) const
{
  size_t i = 0;
  for (auto const& it : *this) {
    if (it.first.name == name) {
      return std::make_optional(i);
    }
    ++i;
  }
  return std::nullopt;
}

std::optional<std::string>
TextureTable::nickname_at_index(size_t const index) const
{
  size_t i = 0;
  for (auto const& it : *this) {
    if (i++ == index) {
      return std::make_optional(it.first.name);
    }
  }
  return std::nullopt;
}

#define FIND_TF(name)                                                                              \
  [&]() {                                                                                          \
    auto const cmp = [&name](auto const& it) { return it.first.name == name; };                    \
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
  ImageDataPointer image_data{pimage, &SOIL_free_image_data};
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
allocate_texture(stlw::Logger &logger, std::string const& filename, GLenum const format,
    GLint const uv_max)
{
  assert(ANYOF(format == GL_RGB, format == GL_RGBA));

  TextureInfo ti;
  ti.gen_texture(logger, 1);
  ti.target = GL_TEXTURE_2D;
  LOG_TRACE_SPRINTF("allocating texture info %s TextureID %u", filename, ti.id());

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
    ti.bind(logger);
    ON_SCOPE_EXIT([&]() { ti.unbind(logger); });

    GpuUploadConfig const guc{format, ti.target, GL_TEXTURE0};
    auto const image_data = TRY_MOVEOUT(upload_image_gpu(logger, filename, guc));
    ti.height = image_data.height;
    ti.width = image_data.width;

    ti.uv_max = uv_max;

    glGenerateMipmap(ti.target);
    LOG_ANY_GL_ERRORS(logger, "glGenerateMipmap");
  }

  return Ok(Texture{MOVE(ti)});
}

TextureResult
upload_3dcube_texture(stlw::Logger &logger, std::vector<std::string> const& paths,
    GLenum const format)
{
  assert(paths.size() == 6);
  assert(ANYOF(format == GL_RGB, format == GL_RGBA));

  static constexpr auto targets = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // back
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, // right
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // front
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // left
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // top
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // bottom
  };

  TextureInfo ti;
  ti.gen_texture(logger, 1);
  ti.target = GL_TEXTURE_CUBE_MAP;

  auto const upload_fn = [&format, &logger, &ti](std::string const& filename, GLenum const target)
    -> Result<stlw::none_t, std::string>
  {
    GpuUploadConfig const guc{format, target, GL_TEXTURE0};
    auto const image_data = TRY_MOVEOUT(upload_image_gpu(logger, filename, guc));

    // Either the height is unset (0) or all height/width are the same.
    assert(ti.height == 0 || ti.height == image_data.height);
    assert(ti.width == 0 || ti.width == image_data.width);

    ti.height = image_data.height;
    ti.width = image_data.width;

    return OK_NONE;
  };
  auto const paths_tuple = std::make_tuple(paths[0], paths[1], paths[2], paths[3], paths[4], paths[5]);

  auto const fn = [&]() {
    stlw::zip(upload_fn, targets.begin(), paths_tuple);

    LOG_ANY_GL_ERRORS(logger, "glGenerateMipmap");
    glGenerateMipmap(ti.target);
  };
  while_bound(logger, ti, fn);

  return Ok(Texture{MOVE(ti)});
}

} // ns opengl::texture
