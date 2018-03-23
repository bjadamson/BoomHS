#pragma once
#include <stlw/auto_resource.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/glew.hpp>
#include <array>
#include <optional>

namespace opengl
{

struct TextureInfo
{
  GLenum mode;
  GLuint id;
  GLint  width = 0, height = 0;

  int uv_max = 0;

  void destroy();

  static size_t constexpr NUM_BUFFERS = 1;
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

  static size_t constexpr NUM_BUFFERS = 1;
};

using Texture = stlw::AutoResource<TextureInfo>;
using FrameBuffer = stlw::AutoResource<FBInfo>;
using ResourceBuffer = stlw::AutoResource<RBInfo>;

struct TextureFilenames
{
  std::string              name;
  std::vector<std::string> filenames;

  auto num_filenames() const { return filenames.size(); }

  MOVE_CONSTRUCTIBLE_ONLY(TextureFilenames);
};

class TextureTable
{
  using pair_t = std::pair<TextureFilenames, Texture>;
  std::vector<pair_t> data_;

public:
  TextureTable() = default;
  MOVE_CONSTRUCTIBLE_ONLY(TextureTable);

  void add_texture(TextureFilenames&&, Texture&&);

  std::optional<TextureInfo> find(std::string const&) const;
};

namespace texture
{

GLint
wrap_mode_from_string(char const*);

Texture
allocate_texture(stlw::Logger& logger, std::string const&, GLint, GLint, GLint);

Texture
upload_3dcube_texture(stlw::Logger&, std::vector<std::string> const&, GLint);

} // namespace texture
} // namespace opengl
