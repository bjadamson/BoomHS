#pragma once

#include <opengl/draw_info.hpp>
#include <opengl/obj.hpp>
#include <opengl/factory.hpp>
#include <opengl/shader.hpp>

#include <boomhs/state.hpp>

#include <stlw/format.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/sized_buffer.hpp>
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
};

// TODO: not final by any means..
struct Assets
{
  ObjCache obj_cache;
  opengl::ShaderPrograms shader_programs;
};

class GpuHandles
{
  std::vector<opengl::DrawInfo> drawinfos_;
  std::vector<char const*> names_;

public:
  GpuHandles() = default;

  std::size_t
  set(char const* name, opengl::DrawInfo &&di)
  {
    auto const pos = drawinfos_.size();
    drawinfos_.emplace_back(MOVE(di));
    names_.emplace_back(name);

    // return the index di was stored in.
    return pos;
  }

  opengl::DrawInfo const&
  get(std::size_t const index) const
  {
    return drawinfos_[index];
  }

  opengl::DrawInfo const&
  get(char const* name) const
  {
    FOR(i, names_.size()) {
      if (names_[i] == name) {
        return get(i);
      }
    }
    auto const fmt = fmt::sprintf("Error could not find asset '%s'", name);
    std::cerr << fmt << "\n";
    std::abort();
  }

  MOVE_CONSTRUCTIBLE_ONLY(GpuHandles);
};

struct DrawHandles {
  GpuHandles handles;

  MOVE_CONSTRUCTIBLE_ONLY(DrawHandles);
};

} // ns boomhs
