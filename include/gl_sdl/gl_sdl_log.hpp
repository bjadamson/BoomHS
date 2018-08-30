#pragma once
#include <common/algorithm.hpp>
#include <common/log.hpp>
#include <common/optional.hpp>

#include <extlibs/glew.hpp>
#include <string>
#include <vector>

namespace gl_sdl
{

struct GlErrors
{
  std::vector<std::string> values;

  static std::optional<GlErrors> retrieve();

  static void clear();

private:
  GlErrors() = default;
  friend struct ErrorLog;
};
std::ostream&
operator<<(std::ostream& stream, GlErrors const& errors);

struct SdlErrors
{
  static constexpr char const* SDL_NO_ERRORS = "none";
  std::string                  value;

  static std::optional<SdlErrors> retrieve();

  static void clear();

private:
  SdlErrors() = default;
  friend struct ErrorLog;
};
std::ostream&
operator<<(std::ostream& stream, SdlErrors const& errors);

struct ErrorLog
{
  GlErrors const  gl;
  SdlErrors const sdl;

  static void clear();

  static void abort_if_any_errors(common::Logger&);

private:
  ErrorLog() = default;
};
std::ostream&
operator<<(std::ostream& stream, ErrorLog const& ge);

std::string
get_shader_log(GLuint const);

std::string
get_program_log(GLuint const);

void
log_any_gl_errors(common::Logger&, std::string const&, int const);

} // namespace gl_sdl

#define LOG_ANY_GL_ERRORS(logger, msg)                                                             \
  do {                                                                                             \
    ::gl_sdl::log_any_gl_errors(logger, msg, __LINE__);                                            \
  } while (0)
