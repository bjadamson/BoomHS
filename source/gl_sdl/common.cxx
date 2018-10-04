#include <gl_sdl/common.hpp>

namespace gl_sdl
{

Result<GlSdl, std::string>
GlSdl::make_default(common::Logger& logger, char const* name, bool const fullscreen,
                                 int const width, int const height)
{
  SDLContext context = TRY_MOVEOUT(SDLGlobalContext::create(logger));
  SDLWindow window  = TRY_MOVEOUT(context->make_window(logger, name, fullscreen, width, height));

  GlSdl combined{MOVE(context), MOVE(window)};
  return OK_MOVE(combined);
}

} // namespace gl_sdl
