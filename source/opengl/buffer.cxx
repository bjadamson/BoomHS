#include <boomhs/obj.hpp>
#include <opengl/buffer.hpp>
#include <opengl/vertex_attribute.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/log.hpp>

#include <extlibs/fmt.hpp>
#include <ostream>

using namespace boomhs;
using namespace opengl;

using indices_t  = VertexBuffer::indices_t;
using vertices_t = VertexBuffer::vertices_t;

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
// PositionsBuffer
PositionsBuffer::PositionsBuffer(vertices_t &&v)
    : vertices(MOVE(v))
{
}

glm::vec3
PositionsBuffer::min() const
{
  glm::vec3 r;

  size_t i = 0;
  r.x = vertices[i++];
  r.y = vertices[i++];
  r.z = vertices[i++];

  while (i < vertices.size()) {
    r.x = std::min(r.x, vertices[i++]);
    r.y = std::min(r.y, vertices[i++]);
    r.z = std::min(r.z, vertices[i++]);
    assert(i <= vertices.size());
  }

  return r;
}

glm::vec3
PositionsBuffer::max() const
{
  glm::vec3 r;

  size_t i = 0;
  r.x = vertices[i++];
  r.y = vertices[i++];
  r.z = vertices[i++];

  while (i < vertices.size()) {
    r.x = std::max(r.x, vertices[i++]);
    r.y = std::max(r.y, vertices[i++]);
    r.z = std::max(r.z, vertices[i++]);

    assert(i <= vertices.size());
  }

  return r;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// VertexBuffer
VertexBuffer::VertexBuffer(BufferFlags const& f)
    : flags(f)
{
}

VertexBuffer
VertexBuffer::create_interleaved(stlw::Logger& logger, ObjData const& data,
                                        BufferFlags const& flags)
{
  LOG_TRACE_SPRINTF("Creating interleaved buffer with flags: %s", flags.to_string());
  VertexBuffer buffer{flags};
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
    assert(num_vertexes >= 3);
    copy_n(data.vertices, 3, a, num_vertexes);

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

PositionsBuffer
VertexBuffer::positions() const
{
  vertices_t values;

  size_t i = 0;
  while (i < vertices.size()) {
    assert(flags.vertices);
    values.emplace_back(vertices[i++]);

    auto const assert_i = [&]() {
      assert(i <= vertices.size());
    };

    values.emplace_back(vertices[i++]);
    assert_i();

    values.emplace_back(vertices[i++]);
    assert_i();

    // skip over other attributes
    if (flags.normals) {
      i += 3;
      assert_i();
    }
    if (flags.colors) {
      i += 4;
      assert_i();
    }
    if (flags.uvs) {
      i += 2;
      assert_i();
    }
  }
  return PositionsBuffer{MOVE(values)};
}

std::string
VertexBuffer::to_string() const
{
  return fmt::sprintf("{vertices size: %u, indices size: %u}", vertices.size(), indices.size());
}

} // ns opengl
