#pragma once
#include <stlw/type_macros.hpp>
#include <opengl/obj.hpp>
#include <opengl/shape.hpp>

namespace boomhs {

struct Objs {
  opengl::obj house_obj;
  opengl::obj hashtag_obj;

  MOVE_CONSTRUCTIBLE_ONLY(Objs);
};

template<typename HOUSE_UV, typename HASHTAG, typename TILEMAP_HANDLE>
struct GpuBuffers {
  HOUSE_UV house;
  HASHTAG hashtag;
  TILEMAP_HANDLE tilemap_handle;

  MOVE_CONSTRUCTIBLE_ONLY(GpuBuffers);
};

template<typename HOUSE_UV, typename HASHTAG, typename TILEMAP_HANDLE>
struct assets {
  Objs objects;
  GpuBuffers<HOUSE_UV, HASHTAG, TILEMAP_HANDLE> buffers;

  MOVE_CONSTRUCTIBLE_ONLY(assets);
};

template<typename HOUSE_UV, typename HASHTAG, typename TILEMAP_HANDLE>
auto
make_assets(opengl::obj &&house_obj, opengl::obj &&hashtag_obj, HOUSE_UV &&huv, HASHTAG &&ht,
    TILEMAP_HANDLE &&tilemap_handle)
{
  Objs objs{MOVE(house_obj), MOVE(hashtag_obj)};
  GpuBuffers<HOUSE_UV, HASHTAG, TILEMAP_HANDLE> buffers{MOVE(huv), MOVE(ht), MOVE(tilemap_handle)};
  return assets<HOUSE_UV, HASHTAG, TILEMAP_HANDLE>{MOVE(objs), MOVE(buffers)};
}

} // ns boomhs
