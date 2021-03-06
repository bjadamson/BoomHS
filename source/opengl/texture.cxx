#include <gl_sdl/gl_sdl_log.hpp>
#include <opengl/global.hpp>
#include <opengl/texture.hpp>

#include <common/algorithm.hpp>
#include <common/tuple.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/glew.hpp>
#include <extlibs/soil.hpp>

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace opengl;

namespace
{

Result<ImageData, std::string>
upload_image_gpu(common::Logger& logger, std::string const& path, GLenum const target,
                 GLenum const format)
{
  auto image_data = TRY_MOVEOUT(texture::load_image(logger, path.c_str(), format));

  auto const  width  = image_data.width;
  auto const  height = image_data.height;
  auto const* data   = image_data.data.get();

  // The "internal" format and the data format should match so opengl doesn't need to do runtime
  // conversions.
  //
  // I'm not sure why the internal format is an integer, where as the data format
  // (represented by format here) is a GLenum.
  // https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
  GLint const internal_format = format;

  LOG_TRACE_SPRINTF("uploading %s with w: %i, h: %i", path, width, height);
  glTexImage2D(target, 0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

  return OK_MOVE(image_data);
}

} // namespace

namespace opengl
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// TextureInfo
TextureInfo::TextureInfo()
    : target(GL_TEXTURE_2D)
{
}

void
TextureInfo::gen_texture(common::Logger& logger, GLsizei const num)
{
  glGenTextures(num, &id);
  LOG_ANY_GL_ERRORS(logger, "glGenTextures");
}

void
TextureInfo::bind_impl(common::Logger& logger)
{
  global::texture_bind(*this);
}

void
TextureInfo::unbind_impl(common::Logger& logger)
{
  // This is an expected no-op operation.
  //
  // Make sure this isn't masking any problems, try commenting out and see if rendering changes.
  glActiveTexture(GL_TEXTURE0);

  global::texture_unbind(*this);
}

void
TextureInfo::destroy_impl()
{
  glDeleteTextures(TextureInfo::NUM_BUFFERS, &id);
}

GLint
TextureInfo::get_fieldi(GLenum const name)
{
  DEBUG_ASSERT_BOUND(*this);

  GLint value;
  glGetTexParameteriv(this->target, name, &value);
  return value;
}

void
TextureInfo::set_fieldi(GLenum const name, GLint const value)
{
  DEBUG_ASSERT_BOUND(*this);

  glTexParameteri(this->target, name, value);
}

std::string
TextureInfo::to_string() const
{
  return fmt::sprintf("(TextureInfo) id: %u, target: %i, (w, h) : (%i, %i), uv_max: %f", id, target,
                      width, height, uv_max);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// TextureTable
void
TextureTable::add_texture(TextureFilenames&& tf, Texture&& ta)
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

} // namespace opengl

namespace opengl::texture
{

ImageResult
load_image(common::Logger& logger, char const* path, GLenum const format)
{
  int w = 0, h = 0;

  int const      soil_format = format == GL_RGBA ? SOIL_LOAD_RGBA : SOIL_LOAD_RGB;
  unsigned char* pimage      = SOIL_load_image(path, &w, &h, 0, soil_format);
  if (nullptr == pimage) {
    auto const fmt =
        fmt::sprintf("image at path '%s' failed to load, reason '%s'", path, SOIL_last_result());
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
upload_2d_texture(common::Logger& logger, std::string const& filename, TextureInfo&& ti)
{
  GLenum const format = ti.format;
  GLint const  uv_max = ti.uv_max;

  assert(common::or_all(format == GL_RGB, format == GL_RGBA));

  ti.gen_texture(logger, 1);
  ti.target = GL_TEXTURE_2D;
  LOG_TRACE_SPRINTF("allocating texture info %s TextureID %u", filename, ti.id);

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
    BIND_UNTIL_END_OF_SCOPE(logger, ti);

    auto const image_data = TRY_MOVEOUT(upload_image_gpu(logger, filename, ti.target, format));
    ti.height             = image_data.height;
    ti.width              = image_data.width;

    ti.uv_max = uv_max;

    glGenerateMipmap(ti.target);
    LOG_ANY_GL_ERRORS(logger, "glGenerateMipmap");
  }

  return Ok(Texture{MOVE(ti)});
}

TextureResult
upload_3dcube_texture(common::Logger& logger, std::vector<std::string> const& paths,
                      TextureInfo&& ti)
{
  auto const format = ti.format;
  assert(paths.size() == 6);
  assert(common::or_all(format == GL_RGB, format == GL_RGBA));

  ti.gen_texture(logger, 1);
  ti.target = GL_TEXTURE_CUBE_MAP;

  auto const upload_fn = [&](std::string const& filename,
                             GLenum const       target) -> Result<common::none_t, std::string> {
    auto const image_data = TRY_MOVEOUT(upload_image_gpu(logger, filename, target, format));

    // Either the height is unset (0) or all height/width are the same.
    assert(ti.height == 0 || ti.height == image_data.height);
    assert(ti.width == 0 || ti.width == image_data.width);

    ti.height = image_data.height;
    ti.width  = image_data.width;

    return OK_NONE;
  };
  auto const paths_tuple =
      std::make_tuple(paths[0], paths[1], paths[2], paths[3], paths[4], paths[5]);

  auto const fn = [&]() {
    static constexpr auto CUBE_3D_TARGETS = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // back
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, // right
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // front
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // left
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // top
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // bottom
    };

    common::tuple_zip(upload_fn, CUBE_3D_TARGETS.begin(), paths_tuple);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    LOG_ANY_GL_ERRORS(logger, "glGenerateMipmap");
  };
  ti.while_bound(logger, fn);

  return Ok(Texture{MOVE(ti)});
}

} // namespace opengl::texture
