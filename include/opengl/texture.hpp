#pragma once
#include <opengl/bind.hpp>

#include <common/auto_resource.hpp>
#include <common/log.hpp>
#include <common/result.hpp>
#include <common/type_macros.hpp>

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
  void bind_impl(common::Logger&);
  void unbind_impl(common::Logger&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();

  void destroy_impl();

  void gen_texture(common::Logger&, GLsizei);

  GLint get_fieldi(GLenum);
  void  set_fieldi(GLenum, GLint);

  bool        is_2d() const { return target == GL_TEXTURE_2D; }
  std::string to_string() const;
  static size_t constexpr NUM_BUFFERS = 1;
};

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

using Texture = common::AutoResource<TextureInfo>;
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
load_image(common::Logger&, char const*, GLenum const);

GLint
wrap_mode_from_string(char const*);

TextureResult
upload_2d_texture(common::Logger&, std::string const&, TextureInfo&&);

TextureResult
upload_3dcube_texture(common::Logger&, std::vector<std::string> const&, TextureInfo&&);

} // namespace opengl::texture
