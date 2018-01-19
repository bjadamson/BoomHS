#pragma once
#include <opengl/draw_info.hpp>
#include <opengl/obj.hpp>
#include <opengl/factory.hpp>
#include <opengl/lighting.hpp>
#include <opengl/texture.hpp>

#include <stlw/format.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/sized_buffer.hpp>

#include <boost/algorithm/string.hpp>
#include <array>
#include <string>
#include <vector>
#include <utility>

namespace boomhs
{

class ObjCache
{
  using pair_t = std::pair<std::string, opengl::obj>;
  std::vector<pair_t> objects_;
public:
  ObjCache() = default;
  MOVE_CONSTRUCTIBLE_ONLY(ObjCache);

  void
  add_obj(std::string const& name, opengl::obj &&o)
  {
    auto pair = std::make_pair(name, MOVE(o));
    objects_.emplace_back(MOVE(pair));
  }

  void
  add_obj(char const* name, opengl::obj &&o)
  {
    add_obj(std::string{name}, MOVE(o));
  }

  auto const&
  get_obj(char const* name) const
  {
    auto const cmp = [&name](auto const& pair) {
      return pair.first == name;
    };
    auto const it = std::find_if(objects_.cbegin(), objects_.cend(), cmp);

    // for now, assume all queries are found
    assert(it != objects_.cend());

    // yield reference to data
    return it->second;
  }

  auto const&
  get_obj(std::string const& s) const
  {
    return get_obj(s.c_str());
  }
};

enum GeometryType
{
  Cube = 0,
  Mesh,
};

inline GeometryType
from_string(std::string &string)
{
  boost::to_lower(string);
  if (string == "cube") {
    return Cube;
  } else if (string == "mesh") {
    return Mesh;
  }
  std::abort();
}

struct EntityInfo
{
  Transform const transform;
  GeometryType const type;

  // THOUGHT: It doesn't make sense to have a "color" but not a "shader".
  //
  // We lost our compile time guarantees, how to compensate?
  boost::optional<std::string> const shader;
  boost::optional<std::string> const mesh_name;
  boost::optional<opengl::Color> const color;
  boost::optional<opengl::TextureInfo> const texture;
};

struct LoadedEntities
{
  std::vector<std::uint32_t> data;

  BEGIN_END_FORWARD_FNS(data);
};

// TODO: not final by any means..
struct Assets
{
  ObjCache obj_cache;
  LoadedEntities loaded_entities;
  opengl::TextureTable texture_table;

  opengl::GlobalLight global_light;
};

class GpuHandleList
{
  std::vector<opengl::DrawInfo> drawinfos_;
  std::vector<std::uint32_t> entities_;

public:
  GpuHandleList() = default;

  std::size_t
  add(std::uint32_t const entity, opengl::DrawInfo &&di)
  {
    auto const pos = drawinfos_.size();
    drawinfos_.emplace_back(MOVE(di));
    entities_.emplace_back(entity);

    // return the index di was stored in.
    return pos;
  }

  opengl::DrawInfo const&
  get(std::uint32_t const entity) const
  {
    FOR(i, entities_.size()) {
      if (entities_[i] == entity) {
        return drawinfos_[i];
      }
    }
    std::cerr << fmt::format("Error could not find gpu handle associated to entity {}'\n", entity);
    std::abort();
  }

  MOVE_CONSTRUCTIBLE_ONLY(GpuHandleList);
};

class HandleManager {
  GpuHandleList list_;
public:
  MOVE_CONSTRUCTIBLE_ONLY(HandleManager);
  explicit HandleManager(GpuHandleList &&list)
    : list_(MOVE(list))
  {
  }

  auto&
  lookup(std::uint32_t const entity) const
  {
    return list_.get(entity);
  }
};

} // ns boomhs
