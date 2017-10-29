#pragma once
#include <stlw/type_macros.hpp>

namespace game::boomhs {

template<typename HOUSE_UV, typename HASHTAG>
struct assets {
  HOUSE_UV house_uv;
  HASHTAG hashtag;

  explicit assets(HOUSE_UV &&house, HASHTAG &&hashtag_p)
    : house_uv(MOVE(house))
    , hashtag(MOVE(hashtag_p))
  {
  }

  MOVE_CONSTRUCTIBLE_ONLY(assets);
};

template<typename HOUSE_UV, typename HASHTAG>
auto
make_assets(HOUSE_UV &&huv, HASHTAG &&ht)
{
  return assets<HOUSE_UV, HASHTAG>(MOVE(huv), MOVE(ht));
}

} // ns game::boomhs
