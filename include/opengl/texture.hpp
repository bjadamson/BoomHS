#pragma once
#include <opengl/glew.hpp>

#include <stlw/log.hpp>
#include <boost/optional.hpp>
#include <string>
#include <vector>
#include <utility>

namespace opengl
{

struct texture_info
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
};

class TextureTable
{
  using pair_t = std::pair<TextureFilenames, texture_info>;
  std::vector<pair_t> data_;

  boost::optional<texture_info>
  find_texture(char const* name) const
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
  add_texture(TextureFilenames &&tf, texture_info &&ti)
  {
    auto pair = std::make_pair(MOVE(tf), MOVE(ti));
    data_.emplace_back(MOVE(pair));
  }

  auto
  lookup_texture(char const* name) const
  {
    // TODO: for now assume always find texture
    //assert(find_it != filenames_.cend());
    return find_texture(name);
  }
};

namespace texture
{

texture_info
allocate_texture(stlw::Logger &logger, std::string const&);

texture_info
upload_3dcube_texture(stlw::Logger &, std::vector<std::string> const&);

} // ns texture
} // ns opengl
