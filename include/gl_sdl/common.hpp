#include <gl_sdl/gl_sdl_log.hpp>
#include <gl_sdl/sdl_window.hpp>

#include <common/log.hpp>
#include <common/result.hpp>
#include <common/type_macros.hpp>

#include <string>

namespace gl_sdl
{

struct GlSdl
{
  SDLContext context;
  SDLWindow  window;

  MOVE_CONSTRUCTIBLE_ONLY(GlSdl);

  static Result<GlSdl, std::string>
  make_default(common::Logger&, char const*, bool, int, int);
};

} // namespace gl_sdl
