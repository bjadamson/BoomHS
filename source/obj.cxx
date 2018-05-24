#include <boomhs/components.hpp>
#include <boomhs/obj.hpp>

#include <stlw/algorithm.hpp>

#include <cassert>
#include <extlibs/tinyobj.hpp>

using namespace boomhs;
using namespace opengl;

namespace
{

LoadStatus
load_vertices(tinyobj::index_t const& index, tinyobj::attrib_t const& attrib,
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

///////////////////////////////////////////////////////////////////////////////////////////////////
// ObjData
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
load_objfile(stlw::Logger& logger, char const* objpath, char const* mtlpath)
{
  LOG_TRACE_SPRINTF("Loading objfile: %s mtl: %s", objpath,
                    mtlpath == nullptr ? "nullptr" : mtlpath);

  tinyobj::attrib_t                attrib;
  std::vector<tinyobj::shape_t>    shapes;
  std::vector<tinyobj::material_t> materials;
  std::string                      err;

  bool const load_success = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, objpath, mtlpath);
  if (!load_success) {
    LOG_ERROR_SPRINTF("error loading obj, msg: %s", err);
    std::abort();
  }

  // TODO: for now only loading one mesh exactly
  assert(1 == shapes.size());

  // confirm vertices are triangulated
  assert(0 == (attrib.vertices.size() % 3));
  assert(0 == (attrib.normals.size() % 3));
  assert(0 == (attrib.texcoords.size() % 2));

  ObjData objdata;
  auto&   indices      = objdata.indices;
  objdata.num_vertexes = attrib.vertices.size() / 3;
  /*
  LOG_ERROR_SPRINTF("vertice count %u", num_vertexes);
  LOG_ERROR_SPRINTF("normal count %u", attrib.normals.size());
  LOG_ERROR_SPRINTF("texcoords count %u", attrib.texcoords.size());
  LOG_ERROR_SPRINTF("color count %u", attrib.colors.size());
  */

  // Loop over shapes
  FOR(s, shapes.size())
  {
    // Loop over faces(polygon)
    size_t index_offset = 0;
    FOR(f, shapes[s].mesh.num_face_vertices.size())
    {
      auto const fv = shapes[s].mesh.num_face_vertices[f];

      // Loop over vertices in the face.
      FOR(vi, fv)
      {
        // access to vertex
        tinyobj::index_t const index = shapes[s].mesh.indices[index_offset + vi];

#define LOAD_ATTR(...)                                                                             \
  ({                                                                                               \
    auto const load_status = __VA_ARGS__;                                                          \
    if (load_status != LoadStatus::SUCCESS) {                                                      \
      return Err(load_status);                                                                     \
    }                                                                                              \
  })

        LOAD_ATTR(load_vertices(index, attrib, &objdata.vertices));

        // tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
        // tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
        // tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];
        //

        LOAD_ATTR(load_normals(index, attrib, &objdata.normals));
        LOAD_ATTR(load_uvs(index, attrib, &objdata.uvs));

        LOAD_ATTR(load_colors(LOC::WHITE, &objdata.colors));

#undef LOAD_ATTR
        indices.push_back(indices.size()); // 0, 1, 2, ...
      }
      index_offset += fv;

      // per-face material
      int const face_jaterialid = shapes[s].mesh.material_ids[f];
    }
  }

  LOG_ERROR_SPRINTF("materials size: %lu", materials.size());
  FOR(i, materials.size())
  {
    auto const& material = materials[i];
    LOG_ERROR_SPRINTF("Material name: %s", material.name);
  }

  LOG_DEBUG_SPRINTF("num vertices: %u", objdata.num_vertexes);
  LOG_DEBUG_SPRINTF("vertices.size(): %u", objdata.vertices.size());
  LOG_DEBUG_SPRINTF("colors.size(): %u", objdata.colors.size());
  LOG_DEBUG_SPRINTF("normals.size(): %u", objdata.normals.size());
  LOG_DEBUG_SPRINTF("uvs.size(): %u", objdata.uvs.size());
  LOG_DEBUG_SPRINTF("num indices: %u", objdata.indices.size());
  return OK_MOVE(objdata);
}

LoadResult
load_objfile(stlw::Logger& logger, std::string const& objpath)
{
  return load_objfile(logger, objpath.c_str(), nullptr);
}

} // namespace boomhs
