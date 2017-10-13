#pragma once

namespace game::boomhs {

//using SHIT = opengl::factories::pipeline_shape_pair<opengl::mesh<
    //std::tuple<opengl::vertex_d, opengl::normal_d, opengl::uv_d> >,
    //opengl::pipeline<opengl::texture3d_context> >;

template<typename SHIT>
struct assets {
  SHIT const& house_uv;

  assets(SHIT const& house) :
    house_uv(house)
  {
  }
};

} // ns game::boomhs
