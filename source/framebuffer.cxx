#include <opengl/framebuffer.hpp>

using namespace boomhs;

namespace opengl
{

FBInfo::FBInfo(Dimensions const& d, ScreenSize const& ss)
  : dimensions(d)
  , screen_size(ss)
{
  glGenFramebuffers(1, &id);
}

void
FBInfo::bind_impl(stlw::Logger& logger)
{
  glBindFramebuffer(GL_FRAMEBUFFER, id);
  glViewport(dimensions.x, dimensions.y, dimensions.w, dimensions.h);
}

void
FBInfo::unbind_impl(stlw::Logger& logger)
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, screen_size.width, screen_size.height);
}

void
FBInfo::destroy_impl()
{
  glDeleteFramebuffers(1, &id);
}

std::string
FBInfo::to_string() const
{
  return fmt::sprintf("(FBInfo) id: %u", id);
}

} // ns opengl
