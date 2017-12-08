#pragma once
#include <array>
#include <stlw/type_ctors.hpp>
#include <stlw/type_macros.hpp>
#include <opengl/glew.hpp>
#include <opengl/global.hpp>
#include <opengl/obj.hpp>
#include <opengl/types.hpp>
#include <opengl/draw_info.hpp>

namespace opengl
{

struct MeshProperties
{
  GLenum const draw_mode;

  obj const& object_data;

  explicit MeshProperties(GLenum const dm, obj const& obj)
    : draw_mode(dm)
    , object_data(obj)
  {
  }
};

namespace cube_factory
{

using CubeDimensions = WidthHeightLength;

struct CubeProperties
{
  GLenum const draw_mode;
  CubeDimensions const dimensions;
};

struct ColorProperties {
    std::array<Color, 8> const colors;
  };

struct UVProperties {
};

struct WireframeProperties {
  float const alpha = 1.0f;
};

auto
construct_cube(std::array<float, 32> const& vertices, ColorProperties const &props)
{
  // clang-format off
  auto const& colors = props.colors;
  return stlw::make_array<float>(
      vertices[0], vertices[1], vertices[2], vertices[3],
      colors[0].r, colors[0].g, colors[0].a, colors[0].a,

      vertices[4], vertices[5], vertices[6], vertices[7],
      colors[1].r, colors[1].g, colors[1].a, colors[1].a,

      vertices[8], vertices[9], vertices[10], vertices[11],
      colors[2].r, colors[2].g, colors[2].a, colors[2].a,

      vertices[12], vertices[13], vertices[14], vertices[15],
      colors[3].r, colors[3].g, colors[3].a, colors[3].a,

      vertices[16], vertices[17], vertices[18], vertices[19],
      colors[4].r, colors[4].g, colors[4].a, colors[4].a,

      vertices[20], vertices[21], vertices[22], vertices[23],
      colors[5].r, colors[5].g, colors[5].a, colors[5].a,

      vertices[24], vertices[25], vertices[26], vertices[27],
      colors[6].r, colors[6].g, colors[6].a, colors[6].a,

      vertices[28], vertices[29], vertices[30], vertices[31],
      colors[7].r, colors[7].g, colors[7].a, colors[7].a,

      vertices[32], vertices[33], vertices[34], vertices[35],
      colors[8].r, colors[8].g, colors[8].a, colors[8].a
        );
  // clang-format on
}

auto
construct_cube(std::array<float, 32> const& vertices, UVProperties const &props)
{
  // clang-format off
  return stlw::make_array<float>(
      vertices[0], vertices[1], vertices[2], vertices[3],
      vertices[4], vertices[5], vertices[6], vertices[7],
      vertices[8], vertices[9], vertices[10], vertices[11],
      vertices[12], vertices[13], vertices[14], vertices[15],
      vertices[16], vertices[17], vertices[18], vertices[19],
      vertices[20], vertices[21], vertices[22], vertices[23],
      vertices[24], vertices[25], vertices[26], vertices[27],
      vertices[28], vertices[29], vertices[30], vertices[31],
      vertices[32], vertices[33], vertices[34], vertices[35]
      );
  // clang-format on
}

auto
construct_cube(std::array<float, 32> const& vertices, WireframeProperties const &props)
{
  // clang-format off
  return stlw::make_array<float>(
      vertices[0], vertices[1], vertices[2], vertices[3],
      vertices[4], vertices[5], vertices[6], vertices[7],
      vertices[8], vertices[9], vertices[10], vertices[11],
      vertices[12], vertices[13], vertices[14], vertices[15],
      vertices[16], vertices[17], vertices[18], vertices[19],
      vertices[20], vertices[21], vertices[22], vertices[23],
      vertices[24], vertices[25], vertices[26], vertices[27],
      vertices[28], vertices[29], vertices[30], vertices[31],
      vertices[32], vertices[33], vertices[34], vertices[35]
      );
  // clang-format on
}

auto
make_cube(std::array<float, 32> const& vertices, color_t, Color const& color)
{
  std::array<Color, 8> const colors{
      color, color, color, color,
      color, color, color, color
  };
  ColorProperties const p{colors};
  return construct_cube(vertices, p);
}

auto
make_cube(std::array<float, 32> const& vertices, uv_t)
{
  UVProperties const uv;
  return construct_cube(vertices, uv);
}

auto
make_cube(std::array<float, 32> const& vertices, wireframe_t)
{
  WireframeProperties const wf;
  return construct_cube(vertices, wf);
}

} // ns cube_factory

namespace detail
{
template<typename PIPE, typename VERTICES, typename INDICES>
void
copy_to_gpu(stlw::Logger &logger, PIPE &pipeline, DrawInfo const& dinfo, VERTICES const& vertices,
    INDICES const& indices)
{
  // Activate VAO
  global::vao_bind(pipeline.vao());

  glBindBuffer(GL_ARRAY_BUFFER, dinfo.vbo());
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dinfo.ebo());

  auto const& va = pipeline.va();
  va.upload_vertex_format_to_glbound_vao(logger);

  // copy the vertices
  auto const vertices_size = vertices.size() * sizeof(GLfloat);
  auto const& vertices_data = vertices.data();
  glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices_data, GL_STATIC_DRAW);

  // copy the vertice rendering order
  auto const indices_size = sizeof(GLuint) * indices.size();
  auto const& indices_data = indices.data();
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices_data, GL_STATIC_DRAW);
}

} // ns detail

namespace factories
{

template<typename PIPE, typename ...Args>
auto copy_cube_gpu(stlw::Logger &logger, PIPE &pipeline, cube_factory::CubeProperties const& cprop,
    Args &&... args)
{
  // clang-format off
  static constexpr std::array<GLuint, 14> INDICES = {{
    3, 2, 6, 7, 4, 2, 0,
    3, 1, 6, 5, 4, 1, 0
  }};
  // clang-format on

  using factory_type = typename PIPE::info_t;

  auto const& hw = cprop.dimensions;
  auto const h = hw.height;
  auto const w = hw.width;
  auto const l = hw.length;

  // clang-format off
  auto const arr = stlw::make_array<float>(
   -w, -h, l, 1.0f, // front bottom-left
    w, -h, l, 1.0f, // front bottom-right
    w,  h, l, 1.0f, // front top-right
   -w,  h, l, 1.0f, // front top-left

    -w, -h, -l, 1.0f, // back bottom-left
     w, -h, -l, 1.0f, // back bottom-right
     w,  h, -l, 1.0f, // back top-right
    -w,  h, -l, 1.0f  // back top-left
  );

  auto const v = stlw::make_array<float>(
      arr[8], arr[9], arr[10], arr[11],   // CUBE_ROW_2
      arr[12], arr[13], arr[14], arr[15], // CUBE_ROW_3
      arr[24], arr[25], arr[26], arr[27], // CUBE_ROW_6
      arr[28], arr[29], arr[30], arr[31], // CUBE_ROW_7

      arr[4], arr[5], arr[6], arr[7],     // CUBE_ROW_1,
      arr[0], arr[1], arr[2], arr[3],     // CUBE_ROW_0,
      arr[16], arr[17], arr[18], arr[19], // CUBE_ROW_4,
      arr[20], arr[21], arr[22], arr[23]  // CUBE_ROW_5
      );
  auto const vertices = cube_factory::make_cube(v, factory_type{}, std::forward<Args>(args)...);

  DrawInfo dinfo{cprop.draw_mode, INDICES.size()};
  detail::copy_to_gpu(logger, pipeline, dinfo, vertices, INDICES);
  return dinfo;
}

template<typename P, typename ...Args>
auto make_mesh(stlw::Logger &logger, P &pipeline, MeshProperties &&mprop, Args &&... args)
{
  auto const& indices = mprop.object_data.indices;
  auto const& vertices = mprop.object_data.vertices;

  auto const num_indices = static_cast<GLuint>(mprop.object_data.indices.size());
  DrawInfo dinfo{mprop.draw_mode, num_indices};

  detail::copy_to_gpu(logger, pipeline, dinfo, vertices, indices);
  return dinfo;
}

struct TilemapProperties
{
  GLenum const draw_mode;

  std::vector<float> const& vertices;
  std::vector<uint32_t> const& indices;
};

template<typename TileMap>
auto copy_tilemap_gpu(stlw::Logger &logger, ::opengl::PipelineHashtag3D &pipeline,
    TilemapProperties &&tprops, TileMap const& tile_map)
{
  auto const& vertices = tprops.vertices;
  auto const& indices = tprops.indices;

  // assume (x, y, z, w) all present
  // assume (r, g, b, a) all present
  assert((vertices.size() % 8) == 0);

  // Bind the vao (even before instantiating the DrawInfo)
  global::vao_bind(pipeline.vao());

  auto const num_indices = static_cast<GLuint>(indices.size());
  DrawInfo dinfo{tprops.draw_mode, num_indices};
  auto const ebo = dinfo.ebo();
  auto const vbo = dinfo.vbo();

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

  // Enable the VertexAttributes for this pipeline's VAO.
  auto const& va = pipeline.va();
  va.upload_vertex_format_to_glbound_vao(logger);

  // Calculate how much room the buffers need.
  std::size_t const vertices_num_bytes = (vertices.size() * sizeof(GLfloat));
  glBufferData(GL_ARRAY_BUFFER, vertices_num_bytes, vertices.data(), GL_STATIC_DRAW);

  std::size_t const indices_num_bytes = (indices.size() * sizeof(GLuint));
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_num_bytes, indices.data(), GL_STATIC_DRAW);
  return dinfo;
}

} // ns factories

} // ns opengl
