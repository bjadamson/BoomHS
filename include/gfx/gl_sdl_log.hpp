#pragma once
#include <opengl/glew.hpp>

#include <stlw/log.hpp>
#include <stlw/type_ctors.hpp>
#include <boost/optional.hpp>
#include <string>
#include <vector>

namespace gfx
{

struct GlErrors
{
  std::vector<std::string> values;

  static boost::optional<GlErrors>
  retrieve();

  static void
  clear();
private:
  GlErrors() = default;
  friend struct ErrorLog;
};
std::ostream&
operator<<(std::ostream &stream, GlErrors const& errors);

struct SdlErrors
{
  static constexpr char const* SDL_NO_ERRORS = "none";
  std::string value;

  static boost::optional<SdlErrors>
  retrieve();

  static void clear();

private:
  SdlErrors() = default;
  friend struct ErrorLog;
};
std::ostream&
operator<<(std::ostream &stream, SdlErrors const& errors);

struct ErrorLog
{
  GlErrors const gl;
  SdlErrors const sdl;

  static void
  clear();

  static void
  abort_if_any_errors(std::ostream &stream);

private:
  ErrorLog() = default;
};
std::ostream&
operator<<(std::ostream &stream, ErrorLog const& ge);

std::string
get_shader_log(GLuint const);

std::string
get_program_log(GLuint const);

void
log_any_gl_errors(stlw::Logger &, std::string const &, int const);

} // gfx

#define LOG_ANY_GL_ERRORS(logger, msg)                                                             \
  do {                                                                                             \
    ::gfx::log_any_gl_errors(logger, msg, __LINE__);                                               \
  } while (0)
