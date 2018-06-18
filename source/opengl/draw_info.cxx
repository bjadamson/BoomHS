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

std::string
BufferHandles::to_string() const
{
  return fmt::format("vbo: {}, ebo: {}", vbo(), ebo());
}

std::ostream&
operator<<(std::ostream &stream, BufferHandles const& buffers)
{
  stream << buffers.to_string();
  return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// DrawInfo
DrawInfo::DrawInfo(size_t const num_vertexes, GLuint const num_indices)
  : num_vertexes_(num_vertexes)
  , num_indices_(num_indices)
{
}

DrawInfo::DrawInfo(DrawInfo &&other)
  : num_vertexes_(other.num_vertexes_)
  , num_indices_(other.num_indices_)
  , handles_(MOVE(other.handles_))
  , vao_(MOVE(other.vao_))
{
}

DrawInfo&
DrawInfo::operator=(DrawInfo &&other)
{
  num_vertexes_ = other.num_vertexes_;
  num_indices_ = other.num_indices_;
  other.num_vertexes_ = 0;
  other.num_indices_ = 0;

  handles_ = MOVE(other.handles_);
  vao_ = MOVE(other.vao_);
  return *this;
}

std::string
DrawInfo::to_string() const
{
  auto const& dinfo = *this;

  std::string result;
  result += fmt::format("NumIndices: {}\n", dinfo.num_indices());
  result += "VAO:\n" + dinfo.vao_.to_string() + "\n";
  result += "BufferHandles:\n" + dinfo.handles_.to_string() + "\n";
  result += "EBO contents:\n";
  auto const num_indices = dinfo.num_indices();

  // TODO: maybe need to bind the element buffer before calling glMapBuffer?
  /*
  {
    auto const target = GL_ELEMENT_ARRAY_BUFFER;
    GLuint const*const pmem = static_cast<GLuint const*const>(glMapBuffer(target, GL_READ_ONLY));
    assert(pmem);
    ON_SCOPE_EXIT([]() { assert(true == glUnmapBuffer((target))); });

    FOR(i, num_indices) {
      // (void* + GLuint) -> GLuint*
      GLuint const* p_element = pmem + i;
      result += std::to_string(*p_element) + " ";
    }
    result += "\n";
  }
  */

  /*
  auto const stride = va.stride();
  auto const num_vertexes = dinfo.num_vertexes();

  auto const target = GL_ARRAY_BUFFER;
  GLfloat *pmem = static_cast<GLfloat *>(glMapBuffer(target, GL_READ_ONLY));
  assert(pmem);
  ON_SCOPE_EXIT([]() { assert(glUnmapBuffer(target)); });

  result += fmt::format("vbo id: {} VBO contents:\n", dinfo.vbo());
  result += "VBO stride: '" + std::to_string(stride) + "'\n";
  result += "VBO num_vertexes: '" + std::to_string(num_vertexes) + "'\n";
  result += "VBO contents:\n";

  auto count{0};
  for(auto i = 0u; i < num_vertexes; ++i, ++count) {
    if (count == stride) {
      count = 0;
      result += "\n";
    } else if (i > 0) {
      result += " ";
    }
    result += std::to_string(*(pmem + i));
  }
  */
  return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// EntityDrawHandleMap
size_t
EntityDrawHandleMap::add(EntityID const entity, opengl::DrawInfo &&di)
{
  auto const pos = drawinfos_.size();
  drawinfos_.emplace_back(MOVE(di));
  entities_.emplace_back(entity);

  // return the index di was stored in.
  return pos;
}

#define LOOKUP_IMPLEMENTATION                                                                      \
  FOR(i, entities_.size()) {                                                                       \
    if (entities_[i] == entity) {                                                                  \
      return drawinfos_[i];                                                                        \
    }                                                                                              \
  }                                                                                                \
  LOG_ERROR_FMT("Error could not find entity drawinfo associated to entity {}", entity);           \
  std::abort();

opengl::DrawInfo&
EntityDrawHandleMap::lookup(stlw::Logger &logger, EntityID const entity)
{
  LOOKUP_IMPLEMENTATION
}

opengl::DrawInfo const&
EntityDrawHandleMap::lookup(stlw::Logger &logger, EntityID const entity) const
{
  LOOKUP_IMPLEMENTATION
}
#undef LOOKUP_IMPLEMENTATION


////////////////////////////////////////////////////////////////////////////////////////////////////
// TileDrawHandles
#define LOOKUP_IMPLEMENTATION                                                                      \
  assert(type < TileType::UNDEFINED);                                                              \
  auto const index = static_cast<size_t>(type);                                                    \
  return drawinfos_[index];

opengl::DrawInfo&
TileDrawHandles::lookup(stlw::Logger &logger, TileType const type)
{
  LOOKUP_IMPLEMENTATION
}

opengl::DrawInfo const&
TileDrawHandles::lookup(stlw::Logger &logger, TileType const type) const
{
  LOOKUP_IMPLEMENTATION
}
#undef LOOKUP_IMPLEMENTATION

} // ns opengl
