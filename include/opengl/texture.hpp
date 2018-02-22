#pragma once
#include <opengl/glew.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>
#include <utility>

namespace opengl
{

struct TextureInfo
{
  GLenum mode;
  GLuint id;
};

struct TextureAllocation
{
  TextureInfo info;
  bool should_destroy = false;

  static std::size_t constexpr NUM_BUFFERS = 1;

  TextureAllocation();
  ~TextureAllocation();

  NO_COPY(TextureAllocation);
  NO_MOVE_ASSIGN(TextureAllocation);
  TextureAllocation(TextureAllocation &&);
};

struct TextureFilenames
{
  std::string name;
  std::vector<std::string> filenames;

  auto num_filenames() const { return filenames.size(); }

  MOVE_CONSTRUCTIBLE_ONLY(TextureFilenames);
};

class TextureTable
{
  using pair_t = std::pair<TextureFilenames, TextureAllocation>;
  std::vector<pair_t> data_;

  std::optional<TextureInfo>
  lookup_texture(char const*) const;

public:
  TextureTable() = default;
  MOVE_CONSTRUCTIBLE_ONLY(TextureTable);

  void
  add_texture(TextureFilenames &&, TextureAllocation &&);

  std::optional<TextureInfo>
  find(std::string const&) const;
};

namespace texture
{

TextureAllocation
allocate_texture(stlw::Logger &logger, std::string const&,
    GLint const format);

TextureAllocation
upload_3dcube_texture(stlw::Logger &, std::vector<std::string> const&,
    GLint const format);

} // ns texture
} // ns opengl
