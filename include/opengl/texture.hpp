#pragma once
#include <opengl/glew.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>

#include <boost/optional.hpp>
#include <algorithm>
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

struct TextureFilenames
{
  std::string name;
  std::vector<std::string> filenames;

  auto num_filenames() const { return filenames.size(); }

  // TODO: should this check be made an explicit enum or something somewhere?
  bool is_3dcube() const { return filenames.size() == 6; }
  bool is_2d() const { return filenames.size() == 1; }

  MOVE_CONSTRUCTIBLE_ONLY(TextureFilenames);
};

class TextureTable
{
  using pair_t = std::pair<TextureFilenames, TextureInfo>;
  std::vector<pair_t> data_;

  boost::optional<TextureInfo>
  lookup_texture(char const* name) const
  {
    if (!name) {
      return boost::none;
    }
    auto const cmp = [&name](auto const& it) { return it.first.name == name; };
    auto const it = std::find_if(data_.cbegin(), data_.cend(), cmp);
    return it == data_.cend() ? boost::none : boost::make_optional(it->second);
  }

public:
  TextureTable() = default;
  MOVE_CONSTRUCTIBLE_ONLY(TextureTable);

  void
  add_texture(TextureFilenames &&tf, TextureInfo &&ti)
  {
    auto pair = std::make_pair(MOVE(tf), MOVE(ti));
    data_.emplace_back(MOVE(pair));
  }

  auto
  find(std::string const& name) const
  {
    return lookup_texture(name.c_str());
  }

  boost::optional<TextureInfo>
  find(boost::optional<std::string> const& name) const
  {
    return name ? find(*name) : boost::none;
  }
};

namespace texture
{

TextureInfo
allocate_texture(stlw::Logger &logger, std::string const&);

TextureInfo
upload_3dcube_texture(stlw::Logger &, std::vector<std::string> const&);

} // ns texture
} // ns opengl
