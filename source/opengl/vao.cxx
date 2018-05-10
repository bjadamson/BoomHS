#include <opengl/vao.hpp>
#include <extlibs/fmt.hpp>

namespace opengl
{

std::string
VAO::to_string() const
{
  return fmt::sprintf("(VAO) NUM_BUFFERS: %li, raw: %u",
      VAO::NUM_BUFFERS,
      gl_raw_value());
}

std::ostream&
operator<<(std::ostream& stream, VAO const& vao)
{
  stream << vao.to_string();
  return stream;
}

} // ns opengl
