#include <opengl/texture.hpp>

#include <opengl/global.hpp>
#include <gfx/gl_sdl_log.hpp>

#include <stlw/algorithm.hpp>
#include <stlw/tuple.hpp>

#include <extlibs/fmt.hpp>
#include <extlibs/glew.hpp>
#include <extlibs/soil.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <iostream>

namespace
{

using namespace opengl;
using pimage_t = std::unique_ptr<unsigned char, void (*)(unsigned char*)>;

struct ImageData
{
  int width, height;
  pimage_t data;
};

auto
load_image_into_memory(stlw::Logger &logger, char const* path, bool const alpha)
{
  int w = 0, h = 0;
  auto const format = alpha ? SOIL_LOAD_RGBA : SOIL_LOAD_RGB;
  unsigned char *pimage = SOIL_load_image(path, &w, &h, 0, format);
  if (nullptr == pimage) {
    auto const fmt =
        fmt::sprintf("image at path '%s' failed to load, reason '%s'", path, SOIL_last_result());
    LOG_ERROR(fmt);
    std::abort();
  }
  pimage_t image_data{pimage, &SOIL_free_image_data};
  return ImageData{w, h, MOVE(image_data)};
}

void
upload_image(stlw::Logger &logger, std::string const& filename, GLenum const target,
    GLint const format)
{
  std::string const path = "assets/" + filename;
  bool const alpha = GL_RGBA == format ? true : false;
  auto const image_data = load_image_into_memory(logger, path.c_str(), alpha);

  auto const width = image_data.width;
  auto const height = image_data.height;
  auto const* data = image_data.data.get();

  //std::cerr << "uploading '" << path << "' with w: '" << width << "' h: '" << height << "'\n";
  glTexImage2D(target, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
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
    //std::cerr << "=====\n";
    //std::cerr << "cmp it.first.name: '" << it.first.name << "'\n";
    FOR(i, it.first.filenames.size()) {
      auto const& fn = it.first.filenames[i];
      //std::cerr << "cmp fn[i]: '" << fn << "'\n";
    }
    //std::cerr << "=====\n";
    return it.first.name == name;
  };
  auto const it = std::find_if(data_.cbegin(), data_.cend(), cmp);
  return it == data_.cend() ? std::nullopt : std::make_optional(it->second.resource());
}

} // ns opengl

namespace opengl::texture
{

Texture
allocate_texture(stlw::Logger &logger, std::string const& filename, GLint const format)
{
  assert(ANYOF(format == GL_RGB, format == GL_RGBA));

  GLenum constexpr TEXTURE_MODE = GL_TEXTURE_2D;

  TextureInfo ti;
  ti.mode = TEXTURE_MODE;
  glGenTextures(1, &ti.id);
  //std::cerr << "texture '" << filename << "' has TextureID: '" << ti.id << "'\n";

  global::texture_bind(ti);
  ON_SCOPE_EXIT([&ti]() { global::texture_unbind(ti); });

  // Set texture wrapping to GL_REPEAT (usually basic wrapping method)
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Set texture filtering parameters
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(TEXTURE_MODE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  upload_image(logger, filename, TEXTURE_MODE, format);
  return Texture{MOVE(ti)};
}

Texture
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

  auto const upload_fn = [&logger, &format](std::string const& filename, auto const& target) {
    upload_image(logger, filename, target, format);
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

  return Texture{MOVE(ti)};
}

} // ns opengl::texture
