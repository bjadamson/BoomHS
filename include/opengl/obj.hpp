#pragma once
#include <cassert>
#include <opengl/types.hpp>
#include <stlw/algorithm.hpp>
#include <stlw/sized_buffer.hpp>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

namespace opengl
{

struct obj
{
  stlw::sized_buffer<float> buffer;
  stlw::sized_buffer<int> mesh_ordering;
  stlw::sized_buffer<int> indices;
  //stlw::sized_buffer<int> material_indices;
  unsigned int const num_vertices;
};

auto
load_mesh(char const* path)
{
  Assimp::Importer importer;
  aiScene const* pscene = importer.ReadFile(path,
      aiProcess_MakeLeftHanded        |
      aiProcess_FlipWindingOrder      |
      aiProcess_FlipUVs               |
      aiProcess_PreTransformVertices  |
      aiProcess_CalcTangentSpace      |
      aiProcess_GenSmoothNormals      |
      aiProcess_Triangulate           |
      aiProcess_FixInfacingNormals    |
      aiProcess_FindInvalidData       |
      aiProcess_ValidateDataStructure |
      0);
  assert(nullptr != pscene);
  auto const& scene = *pscene;

  // TODO: for now only loading one mesh exactly
  assert(1 == scene.mNumMeshes);

  auto constexpr NUM_FLOATS_POSITION = 4; // x, y, z, w
  auto constexpr NUM_FLOATS_NORMAL = 3;   // xn, yn, zn
  auto constexpr NUM_FLOATS_UVS = 2;      // u, v
  auto constexpr NUM_FLOATS_IN_ONE_VERTICE = NUM_FLOATS_POSITION + NUM_FLOATS_NORMAL
    + NUM_FLOATS_UVS;

  // calculate number of vertices present so we can allocate buffer
  auto total_number_vertices = 0u, total_number_indices = 0u;
  FOR(i, scene.mNumMeshes) {
    assert(nullptr != scene.mMeshes[i]);
    auto const& mesh = *scene.mMeshes[i];

    // accumulate vertice count
    total_number_vertices += mesh.mNumVertices;

    // Each face has 3 indices (all data is triangulated)
    total_number_indices += 3 * mesh.mNumFaces;
  }
  std::cerr << "total_number_vertices: '" << std::to_string(total_number_vertices) << "'\n";
  std::cerr << "total_number_indices: '" << std::to_string(total_number_indices) << "'\n";

  auto const num_floats = NUM_FLOATS_IN_ONE_VERTICE * total_number_vertices;
  std::cerr << "num_floats total: '" << std::to_string(num_floats) << "'\n";

  stlw::sized_buffer<int> indices{total_number_indices};
  stlw::sized_buffer<int> mesh_ordering{scene.mNumMeshes};
  stlw::sized_buffer<float> buffer{num_floats};

  auto iter = 0u, indice_iter = 0u;
  FOR(i, scene.mNumMeshes) {
    assert(nullptr != scene.mMeshes[i]);
    aiMesh const& mesh = *scene.mMeshes[i];

    // assume meshes are in linear order
    mesh_ordering[i] = i;

    assert(mesh.HasFaces());
    FOR(f, mesh.mNumFaces) {
      auto const& face = mesh.mFaces[f];
      indices[indice_iter++] = face.mIndices[0];
      indices[indice_iter++] = face.mIndices[1];
      indices[indice_iter++] = face.mIndices[2];
    }

    assert(mesh.HasPositions());
    FOR(v, mesh.mNumVertices) {
      auto const pos = mesh.mVertices[v];
      buffer[iter++] = pos.x;
      buffer[iter++] = pos.y;
      buffer[iter++] = pos.z;
      buffer[iter++] = 1.0f;

      //assert(mesh.HasNormals());
      auto const normal = mesh.HasNormals()
        ? mesh.mNormals[v]
        : aiVector3D{1.0, 1.0, 1.0};
      buffer[iter++] = normal.x;
      buffer[iter++] = normal.y;
      buffer[iter++] = normal.z;

      assert(mesh.HasTextureCoords(0));
      auto const uv = mesh.mTextureCoords[0][v];
      buffer[iter++] = uv.x;
      buffer[iter++] = uv.y;
      assert((iter <= num_floats) && (num_floats == buffer.size()));
    }
  }
  // Confirm we stuck in exactly as many floats and indices as calculated.
  assert(iter == num_floats);
  assert(indice_iter == indices.size());

  std::cerr << "return obj, parsed\n" << std::endl;
  return obj{MOVE(buffer), MOVE(mesh_ordering), MOVE(indices), total_number_vertices};
}

} // ns opengl
