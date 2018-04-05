#include <boomhs/obj.hpp>
#include <opengl/buffer.hpp>
#include <opengl/vertex_attribute.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/log.hpp>

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
  // 5 == std::strlen("false");
  static int constexpr MAX_LENGTH = 5;

  auto const print_bool = [&stream](char const* text, bool const v) {
    stream << text;
    stream << ": '";
    stream << std::boolalpha << v;
    stream << "'";
  };

  stream << "{";
  print_bool("vertices", qa.vertices);
  stream << ", ";

  print_bool("colors", qa.colors);
  stream << ", ";

  print_bool("normals", qa.normals);
  stream << ", ";

  print_bool("uvs", qa.uvs);

  stream << "}";
  return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// VertexBuffer
VertexBuffer
VertexBuffer::create_interleaved(stlw::Logger& logger, ObjData const& data,
                                        BufferFlags const& query_attr)
{
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
  auto num_vertices = data.vertices.size();
  auto num_normals  = data.normals.size();
  auto num_colors   = data.colors.size();
  auto num_uvs      = data.uvs.size();

  auto const keep_going = [&]() {
    return ALLOF(num_vertices > 0, num_normals > 0, num_colors > 0, num_uvs > 0);
  };

  size_t a = 0, b = 0, c = 0, d = 0;
  while (keep_going()) {
    assert(!data.vertices.empty());
    copy_n(data.vertices, 4, a, num_vertices);

    if (query_attr.normals) {
      copy_n(data.normals, 3, b, num_normals);
    }

    if (query_attr.colors) {
      // encode assumptions for now
      assert(!query_attr.uvs);
      copy_n(data.colors, 4, c, num_colors);
    }
    if (query_attr.uvs) {
      // encode assumptions for now
      assert(!query_attr.colors);

      copy_n(data.uvs, 2, d, num_uvs);
    }
  }

  assert(num_vertices == 0 || num_vertices == data.vertices.size());
  assert(num_normals == 0 || num_normals == data.normals.size());
  assert(num_colors == 0 || num_colors == data.colors.size());
  assert(num_uvs == 0 || num_uvs == data.uvs.size());

  buffer.indices = data.indices;
  assert(buffer.indices.size() == data.indices.size());

  return buffer;
}

} // ns opengl
