#include <opengl/pipelines.hpp>

namespace
{
  using namespace opengl;

  template<typename R, typename ...Args>
  static stlw::result<R, std::string>
  make_pipeline(char const* vertex_s, char const* fragment_s, VertexAttribute &&va, Args &&... args)
  {
    vertex_shader_filename v{vertex_s};
    fragment_shader_filename f{fragment_s};
    DO_TRY(auto sp, make_shader_program(v, f));
    return R{MOVE(sp), MOVE(va), std::forward<Args>(args)...};
  }
} // ns anonymous

namespace opengl
{

stlw::result<OpenglPipelines, std::string>
load_pipelines(stlw::Logger &logger)
{
  DO_TRY(auto d2color, make_pipeline<PipelineColor2D>("2dcolor.vert", "2dcolor.frag",
        va::vertex_color(logger)));

  DO_TRY(auto d2texture_wall,
      make_pipeline<PipelineTexture2D>("2dtexture.vert", "2dtexture.frag",
        va::vertex_uv2d(logger),
        texture::allocate_texture(logger, IMAGES::WALL)));

  DO_TRY(auto d2texture_container,
      make_pipeline<PipelineTexture2D>("2dtexture.vert", "2dtexture.frag",
        va::vertex_uv2d(logger),
        texture::allocate_texture(logger, IMAGES::CONTAINER)));

  DO_TRY(auto d2wire,
      make_pipeline<PipelineWireframe2D>("wire.vert", "wire.frag",
        va::vertex_only(logger), LIST_OF_COLORS::PINK));

  DO_TRY(auto d3color, make_pipeline<PipelineColor3D>("3dcolor.vert", "3dcolor.frag",
        va::vertex_color(logger)));

  DO_TRY(auto d3hashtag, make_pipeline<PipelineHashtag3D>("3d_hashtag.vert", "3d_hashtag.frag",
        va::vertex_color(logger)));

  DO_TRY(auto d3at, make_pipeline<PipelineAt3D>("3d_at.vert", "3d_at.frag",
        va::vertex_color(logger)));

  DO_TRY(auto d3plus, make_pipeline<PipelinePlus3D>("3d_plus.vert", "3d_plus.frag",
      va::vertex_color(logger)));

  DO_TRY(auto d3arrow, make_pipeline<PipelineArrow3D>("3d_arrow.vert", "3d_arrow.frag",
      va::vertex_color(logger)));

  DO_TRY(auto global_x_axis_arrow, make_pipeline<PipelineAxisArrow3D>("axis_arrow.vert", "axis_arrow.frag",
      va::vertex_color(logger)));
  DO_TRY(auto global_y_axis_arrow, make_pipeline<PipelineAxisArrow3D>("axis_arrow.vert", "axis_arrow.frag",
      va::vertex_color(logger)));
  DO_TRY(auto global_z_axis_arrow, make_pipeline<PipelineAxisArrow3D>("axis_arrow.vert", "axis_arrow.frag",
      va::vertex_color(logger)));

  DO_TRY(auto local_x_axis_arrow, make_pipeline<PipelineAxisArrow3D>("axis_arrow.vert", "axis_arrow.frag",
      va::vertex_color(logger)));
  DO_TRY(auto local_y_axis_arrow, make_pipeline<PipelineAxisArrow3D>("axis_arrow.vert", "axis_arrow.frag",
      va::vertex_color(logger)));
  DO_TRY(auto local_z_axis_arrow, make_pipeline<PipelineAxisArrow3D>("axis_arrow.vert", "axis_arrow.frag",
      va::vertex_color(logger)));

  DO_TRY(auto local_forward_arrow, make_pipeline<PipelineAxisArrow3D>(
        "axis_arrow.vert", "axis_arrow.frag", va::vertex_color(logger)));

  DO_TRY(auto camera_arrow0, make_pipeline<PipelineAxisArrow3D>(
        "axis_arrow.vert", "axis_arrow.frag", va::vertex_color(logger)));
  DO_TRY(auto camera_arrow1, make_pipeline<PipelineAxisArrow3D>(
        "axis_arrow.vert", "axis_arrow.frag", va::vertex_color(logger)));
  DO_TRY(auto camera_arrow2, make_pipeline<PipelineAxisArrow3D>(
        "axis_arrow.vert", "axis_arrow.frag", va::vertex_color(logger)));

  DO_TRY(auto d3cube, make_pipeline<PipelineTextureCube3D>("3d_cubetexture.vert", "3d_cubetexture.frag",
        va::vertex_only(logger),
        texture::upload_3dcube_texture(logger,
          IMAGES::CUBE_FRONT,
          IMAGES::CUBE_RIGHT,
          IMAGES::CUBE_BACK,
          IMAGES::CUBE_LEFT,
          IMAGES::CUBE_TOP,
          IMAGES::CUBE_BOTTOM)));

  DO_TRY(auto d3house, make_pipeline<PipelineTexture3D>("3dtexture.vert", "3dtexture.frag",
        va::vertex_normal_uv3d(logger),
        texture::allocate_texture(logger, IMAGES::HOUSE)));

  DO_TRY(auto d3skybox, make_pipeline<PipelineSkybox3D>("3d_cubetexture.vert", "3d_cubetexture.frag",
        va::vertex_only(logger),
        texture::upload_3dcube_texture(logger,
          IMAGES::SB_FRONT,
          IMAGES::SB_RIGHT,
          IMAGES::SB_BACK,
          IMAGES::SB_LEFT,
          IMAGES::SB_TOP,
          IMAGES::SB_BOTTOM)));

  DO_TRY(auto d3terrain, make_pipeline<PipelineColor3D>("3dcolor.vert", "3dcolor.frag",
        va::vertex_color(logger)));

  DO_TRY(auto d3wire, make_pipeline<PipelineWireframe3D>("3dwire.vert", "wire.frag",
        va::vertex_only(logger), LIST_OF_COLORS::PURPLE));

  Pipeline2D d2{MOVE(d2color), MOVE(d2texture_wall), MOVE(d2texture_container), MOVE(d2wire)};

  Pipeline3D d3{
    MOVE(d3color),
    MOVE(d3hashtag),
    MOVE(d3at),
    MOVE(d3plus),
    MOVE(d3arrow),

    MOVE(global_x_axis_arrow),
    MOVE(global_y_axis_arrow),
    MOVE(global_z_axis_arrow),

    MOVE(local_x_axis_arrow),
    MOVE(local_y_axis_arrow),
    MOVE(local_z_axis_arrow),
    MOVE(local_forward_arrow),

    MOVE(camera_arrow0),
    MOVE(camera_arrow1),
    MOVE(camera_arrow2),

    MOVE(d3cube),
    MOVE(d3house),
    MOVE(d3skybox),
    MOVE(d3terrain),
    MOVE(d3wire)};
  return OpenglPipelines{MOVE(d2), MOVE(d3)};
}

void
BasePipeline::set_uniform_matrix_4fv(stlw::Logger &logger, GLchar const *name, glm::mat4 const &matrix)
{
  auto &p = this->program_;
  p.use(logger);

  auto const loc = this->program_.get_uniform_location(logger, name);
  // https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
  //
  // count:
  // For the matrix (glUniformMatrix*) commands, specifies the number of matrices that are to be
  // modified.
  // This should be 1 if the targeted uniform variable is not an array of matrices, and 1 or more
  // if it is an array of matrices.
  GLsizei constexpr COUNT = 1;
  GLboolean constexpr TRANSPOSE_MATRICES = GL_FALSE;

  LOG_TRACE(fmt::sprintf("sending uniform matrix at loc '%d' with data '%s' to GPU", loc,
        glm::to_string(matrix)));
  glUniformMatrix4fv(loc, COUNT, TRANSPOSE_MATRICES, glm::value_ptr(matrix));
  LOG_ANY_GL_ERRORS(logger, "set_uniform_matrix_4fv");
}

void
BasePipeline::set_uniform_array_4fv(stlw::Logger &logger, GLchar const *name, std::array<float, 4> const &floats)
{
  auto &p = this->program_;
  p.use(logger);

  auto const loc = this->program_.get_uniform_location(logger, name);
  // https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
  //
  // For the vector (glUniform*v) commands, specifies the number of elements that are to be
  // modified.
  // This should be 1 if the targeted uniform variable is not an array, and 1 or more if it is an
  // array.
  GLsizei constexpr COUNT = 1;

  glUniform4fv(loc, COUNT, floats.data());
  LOG_ANY_GL_ERRORS(logger, "set_uniform_array_4fv");
}

void
BasePipeline::set_uniform_array_3fv(stlw::Logger &logger, GLchar const* name, std::array<float, 3> const& array)
{
  auto& p = this->program_;
  p.use(logger);

  // https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
  //
  // For the vector (glUniform*v) commands, specifies the number of elements that are to be
  // modified.
  // This should be 1 if the targeted uniform variable is not an array, and 1 or more if it is an
  // array.
  GLsizei constexpr COUNT = 1;

  auto const loc = this->program_.get_uniform_location(logger, name);
  glUniform3fv(loc, COUNT, array.data());
  LOG_ANY_GL_ERRORS(logger, "set_uniform_array_3fv");
}

void
BasePipeline::use_program(stlw::Logger &logger)
{
  this->program_.use(logger);
}

} // ns opengl
