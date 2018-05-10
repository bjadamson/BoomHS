#pragma once
#include <opengl/bind.hpp>
#include <stlw/auto_resource.hpp>
#include <stlw/compiler.hpp>
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
#ifdef DEBUG_BUILD
  mutable bool bound = false;
#endif

  GLenum mode;
  GLuint id;
  GLint  width = 0, height = 0;
  GLuint num_texture_units = 1;

  float uv_max = 0;

  // constructors
  TextureInfo();

  // methods
  void bind(stlw::Logger&);
  void unbind(stlw::Logger&);
  void destroy();

  GLint get_fieldi(GLenum);
  void  set_fieldi(GLenum, GLint);

  std::string to_string() const;
  static size_t constexpr NUM_BUFFERS = 1;
};

using pimage_t = std::unique_ptr<unsigned char, void (*)(unsigned char*)>;
struct ImageData
{
  int      width, height;
  pimage_t data;
};

// FrameBuffer Info
struct FBInfo
{
#ifdef DEBUG_BUILD
  mutable bool bound = false;
#endif

  GLuint id;
  GLuint color_buffer;

  FBInfo();
  COPY_DEFAULT(FBInfo);
  MOVE_ASSIGNABLE(FBInfo);
  FBInfo(FBInfo&&);

  // methods
  void bind(stlw::Logger&);
  void unbind(stlw::Logger&);
  void destroy();

  std::string to_string() const;

  static size_t constexpr NUM_BUFFERS = 1;
};

// RenderBuffer Info
struct RBInfo
{
  GLuint depth;

  void destroy();
  DEFAULT_CONSTRUCTIBLE(RBInfo);
  COPY_DEFAULT(RBInfo);
  MOVE_ASSIGNABLE(RBInfo);
  RBInfo(RBInfo&&);

  std::string to_string() const;

  static size_t constexpr NUM_BUFFERS = 1;
};

using Texture        = stlw::AutoResource<TextureInfo>;
using FrameBuffer    = stlw::AutoResource<FBInfo>;
using ResourceBuffer = stlw::AutoResource<RBInfo>;

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

} // namespace opengl

namespace opengl::texture
{

using ImageResult   = Result<ImageData, std::string>;
using TextureResult = Result<Texture, std::string>;

ImageResult
load_image(stlw::Logger&, char const*, GLint const);

struct GpuUploadConfig
{
  GLenum const target;
  GLenum const format;
};
Result<ImageData, std::string>
upload_image_gpu(stlw::Logger& logger, std::string const& path, GpuUploadConfig const&);

GLint
wrap_mode_from_string(char const*);

TextureResult
allocate_texture(stlw::Logger&, std::string const&, GLenum, GLint);

TextureResult
upload_3dcube_texture(stlw::Logger&, std::vector<std::string> const&, GLenum);

} // namespace opengl::texture
