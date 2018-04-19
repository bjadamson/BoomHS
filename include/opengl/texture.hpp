#pragma once
#include <stlw/auto_resource.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <array>
#include <extlibs/glew.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace opengl
{

struct TextureInfo
{
  GLenum mode;
  GLuint id;
  GLint  width = 0, height = 0;

  int uv_max = 0;

  void destroy();

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

class TextureTable
{
  using pair_t = std::pair<TextureFilenames, Texture>;
  std::vector<pair_t> data_;

public:
  TextureTable() = default;
  MOVE_CONSTRUCTIBLE_ONLY(TextureTable);

  void add_texture(TextureFilenames&&, Texture&&);

  std::optional<TextureFilenames> lookup_nickname(std::string const&) const;
  std::optional<TextureInfo> find(std::string const&) const;
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
allocate_texture(stlw::Logger&, std::string const&, GLint, GLint, GLint);

TextureResult
upload_3dcube_texture(stlw::Logger&, std::vector<std::string> const&, GLint);

} // namespace opengl::texture
