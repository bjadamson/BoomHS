#pragma once
#include <opengl/auto_resource.hpp>
#include <extlibs/glew.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <optional>

namespace opengl
{

struct TextureInfo
{
  GLenum mode;
  GLuint id;

  void deallocate();

  static size_t constexpr NUM_BUFFERS = 1;
};

// FrameBuffer Info
struct FBInfo
{
  GLuint id;
  GLuint color_buffer;

  void deallocate();
  DEFAULT_CONSTRUCTIBLE(FBInfo);
  COPY_DEFAULT(FBInfo);
  MOVE_ASSIGNABLE(FBInfo);
  FBInfo(FBInfo &&);

  static size_t constexpr NUM_BUFFERS = 1;
};

// RenderBuffer Info
struct RBInfo
{
  GLuint depth;

  void deallocate();
  DEFAULT_CONSTRUCTIBLE(RBInfo);
  COPY_DEFAULT(RBInfo);
  MOVE_ASSIGNABLE(RBInfo);
  RBInfo(RBInfo &&);

  static size_t constexpr NUM_BUFFERS = 1;
};

using Texture = AutoResource<TextureInfo>;
using FrameBuffer = AutoResource<FBInfo>;
using ResourceBuffer = AutoResource<RBInfo>;

struct TextureFilenames
{
  std::string name;
  std::vector<std::string> filenames;

  auto num_filenames() const { return filenames.size(); }

  MOVE_CONSTRUCTIBLE_ONLY(TextureFilenames);
};

class TextureTable
{
  using pair_t = std::pair<TextureFilenames, Texture>;
  std::vector<pair_t> data_;

  std::optional<TextureInfo>
  lookup_texture(char const*) const;

public:
  TextureTable() = default;
  MOVE_CONSTRUCTIBLE_ONLY(TextureTable);

  void
  add_texture(TextureFilenames &&, Texture &&);

  std::optional<TextureInfo>
  find(std::string const&) const;
};

namespace texture
{

Texture
allocate_texture(stlw::Logger &logger, std::string const&,
    GLint const format);

Texture
upload_3dcube_texture(stlw::Logger &, std::vector<std::string> const&,
    GLint const format);

} // ns texture
} // ns opengl
