#include <opengl/draw_info.hpp>
#include <opengl/buffer.hpp>
#include <opengl/factory.hpp>
#include <opengl/gpu.hpp>
#include <opengl/shader.hpp>
#include <opengl/shapes.hpp>

#include <boomhs/components.hpp>
#include <boomhs/obj_store.hpp>

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
  assert(this != &other);

  other.vbo_ = 0;
  other.ebo_ = 0;
}

BufferHandles&
BufferHandles::operator=(BufferHandles &&other)
{
  assert(this != &other);

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
  assert(this != &other);
}

DrawInfo&
DrawInfo::operator=(DrawInfo &&other)
{
  assert(this != &other);

  num_vertexes_ = other.num_vertexes_;
  num_indices_ = other.num_indices_;
  other.num_vertexes_ = 0;
  other.num_indices_ = 0;

  handles_ = MOVE(other.handles_);
  vao_ = MOVE(other.vao_);
  return *this;
}

void
DrawInfo::bind_impl(stlw::Logger& logger)
{
  vao().bind_impl(logger);

}

void
DrawInfo::unbind_impl(stlw::Logger& logger)
{
  vao().unbind_impl(logger);
}

std::string
DrawInfo::to_string() const
{
  auto const& dinfo = *this;

  std::string result;
  result += fmt::format("NumIndices: {} ", dinfo.num_indices());
  result += "VAO: " + dinfo.vao().to_string() + " ";
  result += "BufferHandles: {" + dinfo.handles_.to_string() + "} ";
  //auto const num_indices = dinfo.num_indices();

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
EntityDrawHandleMap::add(EntityID const eid, opengl::DrawInfo &&di)
{
  auto const pos = drawinfos_.size();
  drawinfos_.emplace_back(MOVE(di));
  entities_.emplace_back(eid);

  // return the index di was stored in.
  return pos;
}

bool
EntityDrawHandleMap::has(EntityID const eid) const
{
  return lookup(eid);
}

#define LOOKUP_IMPLEMENTATION                                                                      \
  FOR(i, entities_.size()) {                                                                       \
    if (entities_[i] == eid) {                                                                     \
      return &drawinfos_[i];                                                                       \
    }                                                                                              \
  }                                                                                                \
  return nullptr;

opengl::DrawInfo*
EntityDrawHandleMap::lookup(EntityID const eid)
{
  LOOKUP_IMPLEMENTATION
}

opengl::DrawInfo const*
EntityDrawHandleMap::lookup(EntityID const eid) const
{
  LOOKUP_IMPLEMENTATION
}
#undef LOOKUP_IMPLEMENTATION

////////////////////////////////////////////////////////////////////////////////////////////////////
// DrawHandleManager
void
DrawHandleManager::add_entity(EntityID const eid, DrawInfo&& dinfo)
{
  assert(!entities_.has(eid));
  entities_.add(eid, MOVE(dinfo));
}

EntityDrawHandleMap &
DrawHandleManager::entities()
{
  return entities_;
}

EntityDrawHandleMap const&
DrawHandleManager::entities() const
{
  return entities_;
}

// TODO: When the maps can be indexed by a single identifier (right now they cannot, as the
// EntityID is duplicated in both DrawHandle maps (regular entities and bbox entities).
// Once complete, this macro should look through the various maps for the matching eid.
#define LOOKUP_MANAGER_IMPL(MAP)                                                                   \
  auto *p = MAP.lookup(eid);                                                                       \
  if (p) {                                                                                         \
    return *p;                                                                                     \
  }                                                                                                \
  LOG_ERROR_FMT("Error could not find entity drawinfo associated to entity {}", eid);              \
  std::abort();                                                                                    \
  return *p;

DrawInfo&
DrawHandleManager::lookup_entity(stlw::Logger& logger, EntityID const eid)
{
  LOOKUP_MANAGER_IMPL(entities());
}

DrawInfo const&
DrawHandleManager::lookup_entity(stlw::Logger& logger, EntityID const eid) const
{
  LOOKUP_MANAGER_IMPL(entities());
}

#undef LOOKUP_MANAGER_IMPL

void
DrawHandleManager::add_mesh(stlw::Logger& logger, ShaderPrograms& sps, ObjStore& obj_store,
                            EntityID const eid, EntityRegistry& registry)
{
  auto& sn = registry.get<ShaderName>(eid);
  auto& va = sps.ref_sp(sn.value).va();

  auto const& mesh_name     = registry.get<MeshRenderable>(eid).name;
  auto const& obj = obj_store.get(logger, mesh_name);

  auto handle = opengl::gpu::copy_gpu(logger, va, obj);
  add_entity(eid, MOVE(handle));

  auto const     posbuffer = obj.positions();
  auto const&    min       = posbuffer.min();
  auto const&    max       = posbuffer.max();
  AABoundingBox::add_to_entity(eid, registry, min, max);
}

void
DrawHandleManager::add_cube(stlw::Logger& logger, ShaderPrograms& sps, EntityID const eid,
                            EntityRegistry& registry)
{
  auto const& cr = registry.get<CubeRenderable>(eid);
  CubeMinMax const cmm{cr.min, cr.max};


  auto& sn = registry.get<ShaderName>(eid);
  auto& va = sps.ref_sp(sn.value).va();

  auto const vertices = OF::cube_vertices(cmm.min, cmm.max);
  auto  handle = opengl::gpu::copy_cube_gpu(logger, vertices, va);
  add_entity(eid, MOVE(handle));

  AABoundingBox::add_to_entity(eid, registry, cr.min, cr.max);
}

} // ns opengl
