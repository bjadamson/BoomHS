#pragma once
#include <opengl/colors.hpp>
#include <opengl/draw_info.hpp>

namespace boomhs
{

class TileGrid;
} // ns boomhs

namespace opengl
{
class ShaderProgram;
struct obj;
} // ns opengl

namespace opengl::factories
{

struct ArrowCreateParams
{
  Color const& color;

  glm::vec3 start;
  glm::vec3 end;

  float const tip_length_factor = 4.0f;
};

struct ArrowEndpoints
{
  glm::vec3 p1;
  glm::vec3 p2;
};

DrawInfo
create_arrow_2d(stlw::Logger &, ShaderProgram const&, ArrowCreateParams &&);

DrawInfo
create_arrow(stlw::Logger &, ShaderProgram const&, ArrowCreateParams &&);

DrawInfo
create_tilegrid(stlw::Logger &, ShaderProgram const&, boomhs::TileGrid const&,
    bool const show_yaxis_lines, Color const& color = LOC::RED);

DrawInfo
create_modelnormals(stlw::Logger &, ShaderProgram const&, glm::mat4 const&,
    obj const&, Color const&);


struct WorldOriginArrows {
  DrawInfo x_dinfo;
  DrawInfo y_dinfo;
  DrawInfo z_dinfo;
};

WorldOriginArrows
create_axis_arrows(stlw::Logger &, ShaderProgram &);

} // ns opengl::factories

namespace OF = opengl::factories;
