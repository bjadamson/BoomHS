#pragma once
#include <boomhs/dimensions.hpp>
#include <opengl/bind.hpp>
#include <stlw/auto_resource.hpp>
#include <stlw/log.hpp>
#include <stlw/result.hpp>
#include <stlw/type_macros.hpp>

#include <extlibs/glew.hpp>
#include <string>

namespace opengl
{

struct FBInfo
{
  DebugBoundCheck          debug_check;
  GLuint                   id;
  boomhs::Dimensions const dimensions;
  boomhs::ScreenSize const screen_size;

  FBInfo(boomhs::Dimensions const&, boomhs::ScreenSize const&);
  NO_COPY(FBInfo);
  MOVE_DEFAULT(FBInfo);

  // methods
  void bind_impl(stlw::Logger&);
  void unbind_impl(stlw::Logger&);
  DEFAULT_WHILEBOUND_MEMBERFN_DECLATION();
  void destroy_impl();

  std::string to_string() const;

  static size_t constexpr NUM_BUFFERS = 1;
};
using FrameBuffer = stlw::AutoResource<FBInfo>;

inline auto
make_fbo(stlw::Logger& logger, boomhs::ScreenSize const& ss)
{
  FBInfo fb{{0, 0, ss.width, ss.height}, ss};
  fb.while_bound(logger, []() { glDrawBuffer(GL_COLOR_ATTACHMENT0); });
  return fb;
}

} // namespace opengl
