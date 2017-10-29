#pragma once

namespace game::boomhs {

//using SHIT = opengl::factories::pipeline_shape_pair<opengl::mesh<
    //std::tuple<opengl::vertex_d, opengl::normal_d, opengl::uv_d> >,
    //opengl::pipeline<opengl::texture3d_context> >;
template<typename HOUSE_UV, typename HASHTAG>
struct assets {
  HOUSE_UV const& house_uv;
  HASHTAG const& hashtag;

  assets(HOUSE_UV const& house, HASHTAG const& hashtag)
    : house_uv(house)
    , hashtag(hashtag)
  {
  }
};

template<typename HOUSE_UV, typename HASHTAG>
auto
make_assets(HOUSE_UV const& huv, HASHTAG const& ht)
{
  return assets<HOUSE_UV, HASHTAG>(huv, ht);
}

} // ns game::boomhs
