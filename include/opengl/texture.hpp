#pragma once
#include <stlw/auto_resource.hpp>
#include <stlw/compiler.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/glew.hpp>
#include <map>
#include <memory>
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

  float uv_max = 0;

  // constructors
  TextureInfo();

  // methods
  void bind() const;
  void unbind() const;

  template <typename FN>
  void while_bound(FN const& fn) const
  {
    bind();
    ON_SCOPE_EXIT([&]() { unbind(); });
    fn();
  }

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
  GLuint id;
  GLuint color_buffer;

  void destroy();
  DEFAULT_CONSTRUCTIBLE(FBInfo);
  COPY_DEFAULT(FBInfo);
  MOVE_ASSIGNABLE(FBInfo);
  FBInfo(FBInfo&&);

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
  std::string              name;
  std::vector<std::string> filenames;

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

GLint
wrap_mode_from_string(char const*);

TextureResult
allocate_texture(stlw::Logger&, std::string const&, GLint, GLint);

TextureResult
upload_3dcube_texture(stlw::Logger&, std::vector<std::string> const&, GLint);

} // namespace opengl::texture
