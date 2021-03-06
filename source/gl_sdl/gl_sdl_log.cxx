#include <gl_sdl/gl_sdl_log.hpp>

#include <common/log.hpp>
#include <extlibs/fmt.hpp>

#include <extlibs/sdl.hpp>
#include <iostream>
#include <string>

namespace
{

// also, make an abstraction over the source, not just vector<char>
static std::string
retrieve(GLuint const handle, void (*f)(GLuint, GLsizei, GLsizei*, GLchar*))
{
  // We have to do dance to get the OpenGL shader logs.
  //
  // 1. Ask OpenGL how buffer space we need to retrieve the buffer.
  // 2. Retrieve the data into our buffer.
  // 3. Return the buffer.
  // Step 1
  GLint log_length = 0;
  glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
  if (0 > log_length) {
    // fix this msg
    return "Error retrieving OpenGL Log info for this shader object.";
  }

  // Step 2
  int const buffer_size = log_length + 1; // +1 for null terminator character '\0'

  // explicitely using parenthesis to not trigger initializer_list ctor
  std::vector<char> buffer(buffer_size);
  f(handle, buffer_size, nullptr, buffer.data());

  // Step 3
  return std::string{buffer.cbegin(), buffer.cend()};
}

} // namespace

namespace gl_sdl
{

///////////////////////////////////////////////////////////////////////////////////////////////////
std::optional<GlErrors>
GlErrors::retrieve()
{
  std::vector<std::string> gl;
  for (GLenum error_code = glGetError();; error_code = glGetError()) {
    if (error_code == GL_NO_ERROR) {
      break;
    }
    std::string e = reinterpret_cast<char const*>(gluErrorString(error_code));
    gl.emplace_back(MOVE(e));
  }
  if (gl.empty()) {
    return std::nullopt;
  }
  return GlErrors{MOVE(gl)};
}

void
GlErrors::clear()
{
  while (glGetError() != GL_NO_ERROR) {
  }
  SDL_ClearError();
}

std::ostream&
operator<<(std::ostream& stream, GlErrors const& errors)
{
  stream << "{";
  for (auto const& e : errors.values) {
    stream << e << "\n";
  }
  stream << "}";
  return stream;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::optional<SdlErrors>
SdlErrors::retrieve()
{
  auto sdl = SDL_GetError();
  assert(nullptr != sdl);
  if (0 == ::strlen(sdl)) {
    return std::nullopt;
  }
  return SdlErrors{MOVE(sdl)};
}

void
SdlErrors::clear()
{
  SDL_ClearError();
}

std::ostream&
operator<<(std::ostream& stream, SdlErrors const& errors)
{
  stream << "{" << errors.value << "}";
  return stream;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void
ErrorLog::abort_if_any_errors(common::Logger& logger)
{
  auto const write_errors = [&logger](char const* prefix, auto const& error) {
    LOG_ERROR_SPRINTF("%s detected: %s\n", prefix, error);
    std::abort();
  };
  auto const gl_errors = GlErrors::retrieve();
  if (gl_errors) {
    write_errors("OpenGL Errors", *gl_errors);
  }
  auto const sdl_errors = SdlErrors::retrieve();
  if (sdl_errors) {
    write_errors("SDL Errors", *sdl_errors);
  }
}

void
ErrorLog::clear()
{
  SdlErrors::clear();
  GlErrors::clear();
}

std::ostream&
operator<<(std::ostream& stream, ErrorLog const& ge)
{
  stream << "Graphics errors:\n";
  stream << "    OpenGL errors: '" << ge.gl << "'\n";
  stream << "    SDL errors: '" << ge.sdl << "'\n";
  return stream;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
std::string
get_shader_log(GLuint const handle)
{
  return retrieve(handle, glGetShaderInfoLog);
}

std::string
get_program_log(GLuint const handle)
{
  return retrieve(handle, glGetProgramInfoLog);
}

void
log_any_gl_errors(common::Logger& logger, std::string const& prefix, int const line)
{
  GLenum const err = glGetError();
  if (err != GL_NO_ERROR) {
    LOG_ERROR_SPRINTF("PREFIX: '%s', GL error detected (line %d), code: '%d', string: '%s'", prefix,
                      line, err, gluErrorString(err));
    std::abort();
  }
}

} // namespace gl_sdl
