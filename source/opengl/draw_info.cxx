#include <opengl/draw_info.hpp>
#include <stlw/algorithm.hpp>

#include <iostream>

using namespace boomhs;
namespace opengl
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// BufferHandles
BufferHandles::BufferHandles()
{
  glGenBuffers(NUM_BUFFERS, &vbo_);
  glGenBuffers(NUM_BUFFERS, &ebo_);
}

BufferHandles::~BufferHandles()
{
  glDeleteBuffers(NUM_BUFFERS, &ebo_);
  glDeleteBuffers(NUM_BUFFERS, &vbo_);
}

  // move-construction OK.
BufferHandles::BufferHandles(BufferHandles &&other)
    : vbo_(other.vbo_)
    , ebo_(other.ebo_)
{
  other.vbo_ = 0;
  other.ebo_ = 0;
}

BufferHandles&
BufferHandles::operator=(BufferHandles &&other)
{
  vbo_ = MOVE(other.vbo_);
  ebo_ = MOVE(other.ebo_);

  other.vbo_ = 0;
  other.ebo_ = 0;
  return *this;
}

std::ostream&
operator<<(std::ostream &stream, BufferHandles const& buffers)
{
  stream << fmt::format("vbo: {}, ebo: {}", buffers.vbo(), buffers.ebo());;
  return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// DrawInfo
DrawInfo::DrawInfo(GLenum const dm, size_t const num_vertices, GLuint const num_indices,
    std::optional<TextureInfo> const& ti)
  : draw_mode_(dm)
  , num_vertices_(num_vertices)
  , num_indices_(num_indices)
  , texture_info_(ti)
{
}

DrawInfo::DrawInfo(DrawInfo &&other)
  : draw_mode_(other.draw_mode_)
  , num_vertices_(other.num_vertices_)
  , num_indices_(other.num_indices_)
  , handles_(MOVE(other.handles_))
  , vao_(MOVE(other.vao_))
  , texture_info_(MOVE(other.texture_info_))
{
}

DrawInfo&
DrawInfo::operator=(DrawInfo &&other)
{
  draw_mode_ = other.draw_mode_;
  num_vertices_ = other.num_vertices_;
  num_indices_ = other.num_indices_;
  other.draw_mode_ = -1;
  other.num_vertices_ = 0;
  other.num_indices_ = 0;

  handles_ = MOVE(other.handles_);
  vao_ = MOVE(other.vao_);
  texture_info_ = MOVE(other.texture_info_);
  return *this;
}

void
DrawInfo::print_self(std::ostream &stream, VertexAttribute const& va) const
{
  auto const& dinfo = *this;
  stream << fmt::format("DrawMode: {} NumIndices: {}\n", dinfo.draw_mode(), dinfo.num_indices());
  stream << "VAO:\n" << dinfo.vao_ << "\n";
  stream << "BufferHandles:\n" << dinfo.handles_ << "\n";
  stream << "EBO contents:\n";
  auto const num_indices = dinfo.num_indices();
  {
    auto const target = GL_ELEMENT_ARRAY_BUFFER;
    GLuint const*const pmem = static_cast<GLuint const*const>(glMapBuffer(target, GL_READ_ONLY));
    assert(pmem);
    ON_SCOPE_EXIT([]() { glUnmapBuffer((target)); });

    FOR(i, num_indices) {
      // (void* + GLuint) -> GLuint*
      GLuint const* p_element = pmem + i;
      stream << std::to_string(*p_element) << " ";
    }
    stream << "\n";
  }

  auto const stride = va.stride();
  auto const num_vertices = dinfo.num_vertices();

  auto const target = GL_ARRAY_BUFFER;
  GLfloat *pmem = static_cast<GLfloat *>(glMapBuffer(target, GL_READ_ONLY));
  assert(pmem);
  ON_SCOPE_EXIT([]() { assert(glUnmapBuffer(target)); });

  stream << fmt::format("vbo id: {} VBO contents:\n", dinfo.vbo());
  stream << "VBO stride: '" << stride << "'\n";
  stream << "VBO num_vertices: '" << num_vertices << "'\n";
  stream << "VBO contents:\n";

  auto count{0};
  for(auto i = 0u; i < num_vertices; ++i, ++count) {
    if (count == stride) {
      count = 0;
      stream << "\n";
    } else if (i > 0) {
      stream << " ";
    }
    stream << std::to_string(*(pmem + i));
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EntityDrawinfos
size_t
EntityDrawinfos::add(EntityID const entity, opengl::DrawInfo &&di)
{
  auto const pos = drawinfos_.size();
  drawinfos_.emplace_back(MOVE(di));
  entities_.emplace_back(entity);

  // return the index di was stored in.
  return pos;
}

opengl::DrawInfo const&
EntityDrawinfos::get(EntityID const entity) const
{
  FOR(i, entities_.size()) {
    if (entities_[i] == entity) {
      return drawinfos_[i];
    }
  }
  std::cerr << fmt::format("Error could not find entity drawinfo associated to entity {}'\n", entity);
  std::abort();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EntityDrawHandles
opengl::DrawInfo const&
EntityDrawHandles::lookup(EntityID const eid) const
{
  return infos_.get(eid);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EntityDrawHandles
opengl::DrawInfo const&
TileDrawHandles::lookup(TileType const type) const
{
  assert(type < TileType::UNDEFINED);
  auto const index = static_cast<size_t>(type);
  return drawinfos_[index];
}

} // ns opengl
