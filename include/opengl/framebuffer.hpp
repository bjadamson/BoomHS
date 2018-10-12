#pragma once
#include <boomhs/viewport.hpp>
#include <common/auto_resource.hpp>
#include <common/log.hpp>
#include <common/result.hpp>
#include <common/type_macros.hpp>

#include <opengl/bind.hpp>
#include <opengl/renderbuffer.hpp>
#include <opengl/texture.hpp>

#include <extlibs/glew.hpp>
#include <string>

namespace opengl
{

struct FBInfo
{
  DebugBoundCheck     debug_check;
  GLuint              id;
  boomhs::Viewport    view_port;
  boomhs::ScreenSize  screen_size;

  FBInfo(boomhs::Viewport const&, boomhs::ScreenSize const&);
  NO_COPY(FBInfo);
  MOVE_DEFAULT(FBInfo);

  // methods
  void bind_impl(common::Logger&);
  void unbind_impl(common::Logger&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();
  void destroy_impl();

  std::string to_string() const;

  TextureInfo  attach_color_buffer(common::Logger&, int, int, GLenum);
  TextureInfo  attach_depth_buffer(common::Logger&, int, int, GLenum);
  RenderBuffer attach_render_buffer(common::Logger&, int, int);

  static size_t constexpr NUM_BUFFERS = 1;
};
using FrameBuffer = common::AutoResource<FBInfo>;

inline auto
make_fbo(common::Logger& logger, boomhs::Viewport const& vp, boomhs::ScreenSize const& ss)
{
  FBInfo fb{vp, ss};
  fb.while_bound(logger, []() { glDrawBuffer(GL_COLOR_ATTACHMENT0); });
  return fb;
}

} // namespace opengl
