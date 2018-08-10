#include <opengl/framebuffer.hpp>

using namespace boomhs;

namespace opengl
{

FBInfo::FBInfo(ScreenDimensions const& d, ScreenSize const& ss)
    : dimensions(d)
    , screen_size(ss)
{
  glGenFramebuffers(1, &id);
}

void
FBInfo::bind_impl(common::Logger& logger)
{
  glBindFramebuffer(GL_FRAMEBUFFER, id);
  glViewport(dimensions.left(), dimensions.top(), dimensions.right(), dimensions.bottom());
}

void
FBInfo::unbind_impl(common::Logger& logger)
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(dimensions.left(), dimensions.top(), screen_size.width, screen_size.height);
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

} // namespace opengl
