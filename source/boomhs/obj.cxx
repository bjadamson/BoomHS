#include <boomhs/components.hpp>
#include <boomhs/obj.hpp>

#include <common/algorithm.hpp>

#include <cassert>

using namespace boomhs;
using namespace opengl;

namespace
{

LoadStatus
load_positions(tinyobj::index_t const& index, tinyobj::attrib_t const& attrib,
               std::vector<float>* pvertices)
{
  auto const pos_index = 3 * index.vertex_index;
  if (pos_index < 0) {
    return LoadStatus::MISSING_POSITION_ATTRIBUTES;
  }
  auto const x = attrib.vertices[pos_index + 0];
  auto const y = attrib.vertices[pos_index + 1];
  auto const z = attrib.vertices[pos_index + 2];

  auto& vertices = *pvertices;
  vertices.push_back(x);
  vertices.push_back(y);
  vertices.push_back(z);
  return LoadStatus::SUCCESS;
}

LoadStatus
load_normals(tinyobj::index_t const& index, tinyobj::attrib_t const& attrib,
             std::vector<float>* pvertices)
{
  auto const ni = 3 * index.normal_index;
  if (ni >= 0) {
    auto const xn = attrib.normals[ni + 0];
    auto const yn = attrib.normals[ni + 1];
    auto const zn = attrib.normals[ni + 2];

    auto& vertices = *pvertices;
    vertices.emplace_back(-xn);
    vertices.emplace_back(-yn);
    vertices.emplace_back(-zn);
  }
  else {
    return LoadStatus::MISSING_NORMAL_ATTRIBUTES;
  }
  return LoadStatus::SUCCESS;
}

LoadStatus
load_uvs(tinyobj::index_t const& index, tinyobj::attrib_t const& attrib,
         std::vector<float>* pvertices)
{
  auto const ti = 2 * index.texcoord_index;
  if (ti >= 0) {
    auto const u = attrib.texcoords[ti + 0];
    auto const v = 1.0f - attrib.texcoords[ti + 1];

    auto& vertices = *pvertices;
    vertices.emplace_back(u);
    vertices.emplace_back(v);
  }
  else {
    return LoadStatus::MISSING_UV_ATTRIBUTES;
  }
  return LoadStatus::SUCCESS;
}

LoadStatus
load_colors(Color const& color, std::vector<float>* pvertices)
{
  auto& vertices = *pvertices;
  vertices.push_back(color.r());
  vertices.push_back(color.g());
  vertices.push_back(color.b());
  vertices.push_back(color.a());

  return LoadStatus::SUCCESS;
}

} // namespace

namespace boomhs
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// PositionsBuffer
PositionsBuffer::PositionsBuffer(ObjVertices&& v)
    : vertices(MOVE(v))
{
}

glm::vec3
PositionsBuffer::min() const
{
  glm::vec3 r;

  size_t i = 0;
  r.x      = vertices[i++];
  r.y      = vertices[i++];
  r.z      = vertices[i++];

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
  r.x      = vertices[i++];
  r.y      = vertices[i++];
  r.z      = vertices[i++];

  while (i < vertices.size()) {
    r.x = std::max(r.x, vertices[i++]);
    r.y = std::max(r.y, vertices[i++]);
    r.z = std::max(r.z, vertices[i++]);

    assert(i <= vertices.size());
  }

  return r;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ObjData
ObjData
ObjData::clone() const
{
  return *this;
}

PositionsBuffer
ObjData::positions() const
{
  auto copy = vertices;
  return PositionsBuffer{MOVE(copy)};
}

std::string
ObjData::to_string() const
{
  return fmt::sprintf(
      "{num_vertexes: %u, num vertices: %u, num colors: %u, num uvs: %u, num indices: %u}",
      num_vertexes, vertices.size(), colors.size(), normals.size(), uvs.size(), indices.size());
}

// LoadStatus
///////////////////////////////////////////////////////////////////////////////////////////////////

std::string
loadstatus_to_string(LoadStatus const ls)
{
//
// TODO: derive second argument from first somehow?
#define CASE(ATTRIBUTE, ATTRIBUTE_S)                                                               \
  case LoadStatus::ATTRIBUTE:                                                                      \
    return ATTRIBUTE_S;

  // clang-format off
  switch (ls) {
    CASE(MISSING_POSITION_ATTRIBUTES, "MISSING_POSITION_ATTRIBUTES");
    CASE(MISSING_COLOR_ATTRIBUTES,    "MISSING_COLOR_ATTRIBUTES");
    CASE(MISSING_NORMAL_ATTRIBUTES,   "MISSING_NORMAL_ATTRIBUTES");
    CASE(MISSING_UV_ATTRIBUTES,       "MISSING_UV_ATTRIBUTES");

    CASE(TINYOBJ_ERROR,               "TINYOBJ_ERROR");
    CASE(SUCCESS,                     "SUCCESS");
  default:
    break;
  }
  // clang-format on
#undef CASE

  // terminal error
  std::abort();
}

std::ostream&
operator<<(std::ostream& stream, LoadStatus const& ls)
{
  stream << loadstatus_to_string(ls);
  return stream;
}

LoadResult
load_objfile(common::Logger& logger, char const* objpath, char const* mtlpath)
{
  LOG_TRACE_SPRINTF("Loading objfile: %s mtl: %s", objpath,
                    mtlpath == nullptr ? "nullptr" : mtlpath);
  assert(mtlpath);

  tinyobj::attrib_t attrib;
  std::string       err;

  ObjData objdata;
  auto&   materials = objdata.materials;
  auto&   shapes    = objdata.shapes;

  bool const load_success = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, objpath, mtlpath);
  if (!load_success) {
    LOG_ERROR_SPRINTF("error loading obj, msg: %s", err);
    std::abort();
  }

  assert(!objdata.materials.empty());

  // TODO: for now only loading one mesh exactly
  assert(1 == shapes.size());

  // confirm vertices are triangulated
  assert(0 == (attrib.vertices.size() % 3));
  assert(0 == (attrib.normals.size() % 3));
  assert(0 == (attrib.texcoords.size() % 2));

  auto& indices        = objdata.indices;
  objdata.num_vertexes = attrib.vertices.size() / 3;
  /*
  LOG_ERROR_SPRINTF("vertice count %u", num_vertexes);
  LOG_ERROR_SPRINTF("normal count %u", attrib.normals.size());
  LOG_ERROR_SPRINTF("texcoords count %u", attrib.texcoords.size());
  LOG_ERROR_SPRINTF("color count %u", attrib.colors.size());
  */

  LOG_DEBUG_SPRINTF("materials size: %lu", materials.size());
  FOR(i, materials.size())
  {
    auto const& material = materials[i];
    auto const& diffuse  = material.diffuse;
    auto const  color    = Color{diffuse[0], diffuse[1], diffuse[2], 1.0};
    LOG_TRACE_SPRINTF("Material name %s, diffuse %s", material.name, color.to_string());
  }

  auto const get_facecolor = [&materials](auto const& shape, auto const f) {
    // per-face material
    int const   face_materialid = shape.mesh.material_ids[f];
    auto const& diffuse         = materials[face_materialid].diffuse;
    return Color{diffuse[0], diffuse[1], diffuse[2], 1.0};
  };

  size_t     index_offset           = 0;
  auto const load_vertex_attributes = [&](auto const& shape, auto const& face) -> LoadStatus {
    auto const face_color = get_facecolor(shape, face);

    auto const fv = shape.mesh.num_face_vertices[face];
    // Loop over vertices in the face.
    FOR(vi, fv)
    {
      // access to vertex
      tinyobj::index_t const index = shape.mesh.indices[index_offset + vi];

#define LOAD_ATTR(...)                                                                             \
  ({                                                                                               \
    auto const load_status = __VA_ARGS__;                                                          \
    if (load_status != LoadStatus::SUCCESS) {                                                      \
      return load_status;                                                                          \
    }                                                                                              \
  })

      LOAD_ATTR(load_positions(index, attrib, &objdata.vertices));
      LOAD_ATTR(load_normals(index, attrib, &objdata.normals));
      LOAD_ATTR(load_uvs(index, attrib, &objdata.uvs));
      LOAD_ATTR(load_colors(face_color, &objdata.colors));

#undef LOAD_ATTR
      indices.push_back(indices.size()); // 0, 1, 2, ...
    }
    index_offset += fv;

    return LoadStatus::SUCCESS;
  };

  objdata.foreach_face(load_vertex_attributes);

  LOG_DEBUG_SPRINTF("num vertices: %u", objdata.num_vertexes);
  LOG_DEBUG_SPRINTF("vertices.size(): %u", objdata.vertices.size());
  LOG_DEBUG_SPRINTF("colors.size(): %u", objdata.colors.size());
  LOG_DEBUG_SPRINTF("normals.size(): %u", objdata.normals.size());
  LOG_DEBUG_SPRINTF("uvs.size(): %u", objdata.uvs.size());
  LOG_DEBUG_SPRINTF("num indices: %u", objdata.indices.size());
  return OK_MOVE(objdata);
}

LoadResult
load_objfile(common::Logger& logger, std::string const& path, std::string const& name)
{
  auto const objpath = path + name;
  return load_objfile(logger, objpath.c_str(), path.c_str());
}

} // namespace boomhs
