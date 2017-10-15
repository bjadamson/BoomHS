#include <opengl/obj.hpp>

#include <cassert>
#include <opengl/types.hpp>
#include <stlw/algorithm.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobj/tinyobj.hpp>

namespace opengl
{

obj
load_mesh(char const* path)
{
  std::cerr << "load_mesh\n";
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string err;

  bool const load_success = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path);
  if (load_success) {
    std::cerr << "load obj successful.\n";
  } else {
    std::cerr << "load obj failed.\n";
    std::abort();
  }

  if (!err.empty()) {
    std::cerr << "error loading obj, msg: '" << err << "'\n";
    std::abort();
  }
  std::cerr << "obj load successful\n";
  std::cerr << "shapes size: '" << shapes.size() << "'\n";

  // TODO: for now only loading one mesh exactly
  assert(1 == shapes.size());

  // confirm vertices are triangulated
  assert(0 == (attrib.vertices.size() % 3));
  assert(0 == (attrib.normals.size() % 3));
  assert(0 == (attrib.texcoords.size() % 3));

  std::vector<float> buffer;
  std::vector<uint32_t> indices;

  std::cerr << "vertice count '" << (attrib.vertices.size() / 3) << "'\n";
  std::cerr << "normal count '" << attrib.normals.size() << "'\n";
  std::cerr << "texcoords count '" << attrib.texcoords.size() << "'\n";
  std::cerr << "color count '" << attrib.colors.size() << "'\n";

  for (auto const& shape : shapes) {
    for (auto const& index : shape.mesh.indices) {
      auto const x = attrib.vertices[3 * index.vertex_index + 0];
      auto const y = attrib.vertices[3 * index.vertex_index + 1];
      auto const z = attrib.vertices[3 * index.vertex_index + 2];
      auto const w = 1.0f;

      buffer.push_back(x);
      buffer.push_back(y);
      buffer.push_back(z);
      buffer.push_back(w);

      auto const xn = attrib.normals[3 * index.normal_index + 0];
      auto const yn = attrib.normals[3 * index.normal_index + 1];
      auto const zn = attrib.normals[3 * index.normal_index + 2];

      buffer.push_back(xn);
      buffer.push_back(yn);
      buffer.push_back(zn);

      auto const u = attrib.texcoords[2 * index.texcoord_index + 0];
      auto const v = attrib.texcoords[2 * index.texcoord_index + 1];

      buffer.push_back(u);
      buffer.push_back(v);

      indices.push_back(indices.size()); // 0, 1, 2, ...
    }
  }
  std::cerr << "buffer.size() '" << buffer.size() << "'\n";
  std::cerr << "indices.size() '" << buffer.size() << "'\n";
  std::cerr << "return obj, parsed\n" << std::endl;
  return obj{MOVE(buffer), MOVE(indices)};
}

} // ns opengl
