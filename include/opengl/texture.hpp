#pragma once
#include <opengl/bind.hpp>
#include <opengl/renderbuffer.hpp>

#include <stlw/auto_resource.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/glew.hpp>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace opengl
{

struct TextureInfo
{
  DebugBoundCheck debug_check;

  GLenum target = 0;
  GLenum format = 0;
  GLuint id     = 0;
  GLint  wrap   = -1;

  GLint width = 0, height = 0;
  float uv_max = -1.0;

  // constructors
  TextureInfo();
  NO_COPY(TextureInfo);
  MOVE_DEFAULT(TextureInfo);

  // methods
  void bind_impl(stlw::Logger&);
  void unbind_impl(stlw::Logger&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();

  void destroy_impl();

  void gen_texture(stlw::Logger&, GLsizei);

  GLint get_fieldi(GLenum);
  void  set_fieldi(GLenum, GLint);

  bool is_2d() const { return target == GL_TEXTURE_2D; }
  std::string to_string() const;
  static size_t constexpr NUM_BUFFERS = 1;
};

using Texture = stlw::AutoResource<TextureInfo>;

inline TextureInfo
create_texture_attachment(stlw::Logger& logger, int const width, int const height,
                          GLenum const texture_unit)
{
  assert(width > 0 && height > 0);

  TextureInfo ti;
  ti.target = GL_TEXTURE_2D;
  ti.gen_texture(logger, 1);

  glActiveTexture(texture_unit);
  ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });

  ti.while_bound(logger, [&]() {
    // allocate memory for texture
    glTexImage2D(ti.target, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // adjust texture fields
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // attach texture to FBO
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ti.target, ti.id, 0);
  });

  ti.height = height;
  ti.width  = width;

  ti.uv_max = 1.0f;
  return ti;
}

inline auto
create_depth_texture_attachment(stlw::Logger& logger, int const width, int const height,
                                GLenum const texture_unit)
{
  assert(width > 0 && height > 0);

  TextureInfo ti;
  ti.target = GL_TEXTURE_2D;
  ti.gen_texture(logger, 1);

  glActiveTexture(texture_unit);
  ON_SCOPE_EXIT([]() { glActiveTexture(GL_TEXTURE0); });

  ti.while_bound(logger, [&]() {
    ti.set_fieldi(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ti.set_fieldi(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(ti.target, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 nullptr);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, ti.target, ti.id, 0);
  });

  ti.height = height;
  ti.width  = width;

  ti.uv_max = 1.0f;
  return ti;
}

inline auto
create_depth_buffer_attachment(stlw::Logger& logger, int const width, int const height)
{
  RBInfo rbinfo;
  rbinfo.while_bound(logger, [&]() {
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbinfo.id);
  });
  return RenderBuffer{MOVE(rbinfo)};
}

struct TextureFilenames
{
  using StringList = std::vector<std::string>;

  std::string name;
  StringList  filenames;

  auto num_filenames() const { return filenames.size(); }
};

inline bool
operator<(TextureFilenames const& a, TextureFilenames const& b)
{
  return a.name < b.name;
}

class TextureTable
{
  using pair_t = std::pair<TextureFilenames, Texture>;
  std::map<TextureFilenames, Texture> data_;

public:
  TextureTable() = default;
  MOVE_CONSTRUCTIBLE_ONLY(TextureTable);
  BEGIN_END_FORWARD_FNS(data_);

  void add_texture(TextureFilenames&&, Texture&&);

  // Get a concatenated list of all texture names as a single string.
  //
  // Pass in a delimeter character to seperate the names.
  std::string list_of_all_names(char const delim = ' ') const;

  std::optional<size_t>      index_of_nickname(std::string const&) const;
  std::optional<std::string> nickname_at_index(size_t) const;

  TextureFilenames const* lookup_nickname(std::string const&) const;
  TextureInfo*            find(std::string const&);
  TextureInfo const*      find(std::string const&) const;
};

using ImageDataPointer = std::unique_ptr<unsigned char, void (*)(unsigned char*)>;
struct ImageData
{
  int              width, height;
  ImageDataPointer data;
};

} // namespace opengl

namespace opengl::texture
{

using ImageResult   = Result<ImageData, std::string>;
using TextureResult = Result<Texture, std::string>;

ImageResult
load_image(stlw::Logger&, char const*, GLenum const);

GLint
wrap_mode_from_string(char const*);

TextureResult
upload_2d_texture(stlw::Logger&, std::string const&, TextureInfo&&);

TextureResult
upload_3dcube_texture(stlw::Logger&, std::vector<std::string> const&, TextureInfo&&);

} // namespace opengl::texture
