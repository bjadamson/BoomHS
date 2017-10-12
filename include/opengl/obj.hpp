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
      aiProcess_CalcTangentSpace       |
      aiProcess_Triangulate            |
      aiProcess_JoinIdenticalVertices  |
      aiProcess_SortByPType);
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
  auto total_number_vertices = 0u;
  FOR(i, scene.mNumMeshes) {
    assert(nullptr != scene.mMeshes[i]);
    total_number_vertices += scene.mMeshes[i]->mNumVertices;
  }
  std::cerr << "total_number_vertices: '" << std::to_string(total_number_vertices) << "'\n";
  auto const num_floats = NUM_FLOATS_IN_ONE_VERTICE * total_number_vertices;
  std::cerr << "num_floats total: '" << std::to_string(num_floats) << "'\n";

  // one indice for each vertex
  stlw::sized_buffer<int> indices{total_number_vertices};
  stlw::sized_buffer<int> mesh_ordering{scene.mNumMeshes};
  stlw::sized_buffer<float> buffer{num_floats};

  auto iter = 0u, indice_iter = 0u;
  FOR(i, scene.mNumMeshes) {
    assert(nullptr != scene.mMeshes[i]);
    aiMesh const& mesh = *scene.mMeshes[i];

    // assume meshes are in linear order
    mesh_ordering[i] = i;

    assert(mesh.HasFaces());
    FOR(j, mesh.mNumFaces) {
      aiFace const& face = mesh.mFaces[j];

      // Since each triangle has 3 faces, hard-code 3
      assert(face.mNumIndices == 3);
      FOR(k, 3) {
        indices[indice_iter++] = face.mIndices[k];

        assert(mesh.HasPositions());
        auto const pos = mesh.mVertices[face.mIndices[k]];
        buffer[iter++] = pos.x;
        buffer[iter++] = pos.y;
        buffer[iter++] = pos.z;
        buffer[iter++] = 1.0f;

        //assert(mesh.HasNormals());
        auto const normal = mesh.HasNormals()
          ? mesh.mNormals[face.mIndices[k]]
          : aiVector3D{1.0, 1.0, 1.0};
        buffer[iter++] = normal.x;
        buffer[iter++] = normal.y;
        buffer[iter++] = normal.z;

        assert(mesh.HasTextureCoords(0));
        auto const uv = mesh.mTextureCoords[0][face.mIndices[k]];
        buffer[iter++] = uv.x;
        buffer[iter++] = uv.y;
        assert(iter <= num_floats && (num_floats == buffer.size()));
      }
    }
    // Confirm we stuck in exactly as many floats as calculated.
    assert(iter == num_floats);
  }

  // Confirm we stuck in exactly as many indices as calculated.
  assert(indice_iter == indices.size());
  std::cerr << "return obj, parsed\n" << std::endl;
  return obj{MOVE(buffer), MOVE(mesh_ordering), MOVE(indices), total_number_vertices};
}

} // ns opengl
