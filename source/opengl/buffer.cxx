#include <boomhs/obj.hpp>
#include <opengl/buffer.hpp>
#include <opengl/vertex_attribute.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/log.hpp>

#include <extlibs/fmt.hpp>
#include <ostream>

using namespace boomhs;

namespace opengl
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// BufferFlags
BufferFlags
BufferFlags::from_va(VertexAttribute const& va)
{
  bool const p = va.has_vertices();
  bool const n = va.has_normals();
  bool const c = va.has_colors();
  bool const u = va.has_uvs();
  return BufferFlags{p, n, c, u};
}

std::string
BufferFlags::to_string() const
{
  return fmt::sprintf("{vertices: %i, normals: %i, colors: %i, uvs: %i}", vertices, normals,
      colors, uvs);
}

bool
operator==(BufferFlags const& a, BufferFlags const& b)
{
  // clang-format off
  return ALLOF(
    a.vertices == b.vertices,
    a.normals == b.normals,
    b.colors == a.colors,
    a.uvs == b.uvs);
  // clang-format on
}

bool
operator!=(BufferFlags const& a, BufferFlags const& b)
{
  return !(a == b);
}

std::ostream&
operator<<(std::ostream& stream, BufferFlags const& qa)
{
  stream << qa.to_string();
  return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// VertexBuffer
VertexBuffer
VertexBuffer::create_interleaved(stlw::Logger& logger, ObjData const& data,
                                        BufferFlags const& flags)
{
  LOG_TRACE_SPRINTF("Creating interleaved buffer with flags: %s", flags.to_string());
  VertexBuffer buffer;
  auto&        vertices = buffer.vertices;

  auto const copy_n = [&vertices](auto const& buffer, size_t const num, size_t& count,
                                  size_t& remaining) {
    FOR(i, num)
    {
      vertices.emplace_back(buffer[count++]);
      --remaining;
    }
  };
  auto num_vertexes = data.vertices.size();
  auto num_normals  = data.normals.size();
  auto num_colors   = data.colors.size();
  auto num_uvs      = data.uvs.size();

  auto const keep_going = [&]() {
    return num_vertexes > 0;
  };

  size_t a = 0, b = 0, c = 0, d = 0;
  while (keep_going()) {
    assert(!data.vertices.empty());
    assert(num_vertexes >= 4);
    copy_n(data.vertices, 4, a, num_vertexes);

    if (flags.normals) {
      assert(num_normals >= 3);
      copy_n(data.normals, 3, b, num_normals);
    }

    if (flags.colors) {
      // encode assumptions for now
      assert(!flags.uvs);

      assert(num_colors >= 4);
      copy_n(data.colors, 4, c, num_colors);
    }
    if (flags.uvs) {
      // encode assumptions for now
      assert(!flags.colors);

      assert(num_uvs >= 2);
      copy_n(data.uvs, 2, d, num_uvs);
    }
  }

  assert(num_vertexes == 0 || num_vertexes == data.vertices.size());
  assert(num_normals == 0 || num_normals == data.normals.size());
  assert(num_colors == 0 || num_colors == data.colors.size());
  assert(num_uvs == 0 || num_uvs == data.uvs.size());

  buffer.indices = data.indices;
  assert(buffer.indices.size() == data.indices.size());

  LOG_TRACE_SPRINTF("Finished creating interleaved buffer: %s", buffer.to_string());
  return buffer;
}

std::string
VertexBuffer::to_string() const
{
  return fmt::sprintf("{vertices size: %u, indices size: %u}", vertices.size(), indices.size());
}

} // ns opengl
