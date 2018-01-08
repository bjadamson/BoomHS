#pragma once
#include <array>
#include <vector>
#include <utility>
#include <opengl/draw_info.hpp>
#include <opengl/obj.hpp>
#include <opengl/factory.hpp>
#include <boomhs/state.hpp>
#include <stlw/format.hpp>
#include <stlw/log.hpp>
#include <stlw/type_macros.hpp>
#include <stlw/sized_buffer.hpp>

namespace boomhs {

struct Objs {
  opengl::obj house;
  opengl::obj hashtag;
  opengl::obj at;
  opengl::obj plus;
  opengl::obj arrow;

  // Alphabet
  opengl::obj O;
  opengl::obj T;

  MOVE_CONSTRUCTIBLE_ONLY(Objs);
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

struct Assets {
  Objs objects;
  GpuHandles handles;

  MOVE_CONSTRUCTIBLE_ONLY(Assets);
};

} // ns boomhs
