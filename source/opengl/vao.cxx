#include <opengl/vao.hpp>

namespace opengl
{

std::string
VAO::to_string() const
{
  std::string result;
  result += "NUM_BUFFERS: '";
  result += VAO::NUM_BUFFERS;
  result += "' vao_: '";
  result += "' raw_value_:' " + std::to_string(gl_raw_value()) + "'";

  return result;
}

std::ostream&
operator<<(std::ostream& stream, VAO const& vao)
{
  stream << vao.to_string();
  return stream;
}

} // ns opengl
