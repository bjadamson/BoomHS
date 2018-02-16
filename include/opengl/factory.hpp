#pragma once
#include <opengl/colors.hpp>
#include <opengl/draw_info.hpp>
#include <opengl/glew.hpp>
#include <opengl/global.hpp>
#include <opengl/texture.hpp>

#include <stlw/type_macros.hpp>
#include <stlw/type_ctors.hpp>

#include <boomhs/tilegrid.hpp>
#include <boomhs/types.hpp>

#include <stlw/optional.hpp>
#include <array>
#include <cmath>

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

DrawInfo
copy_gpu(stlw::Logger &, GLenum const, ShaderProgram &, obj const&, std::optional<TextureInfo> const&);

} // ns opengl::factories

namespace OF = opengl::factories;
