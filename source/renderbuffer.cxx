#include <opengl/renderbuffer.hpp>

namespace opengl
{

RBInfo::RBInfo()
{
  glGenRenderbuffers(1, &id);
}

void
RBInfo::destroy_impl()
{
  DEBUG_ASSERT_NOT_BOUND(*this);
  glDeleteRenderbuffers(1, &id);
}

void
RBInfo::bind_impl(stlw::Logger &logger)
{
  glBindRenderbuffer(GL_RENDERBUFFER, id);
}

void
RBInfo::unbind_impl(stlw::Logger &logger)
{
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

std::string
RBInfo::to_string() const
{
  return fmt::sprintf("(RBInfo) id: %u", id);
}

} // ns opengl
