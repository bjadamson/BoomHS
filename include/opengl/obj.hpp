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
  stlw::sized_buffer<int> indices;
  unsigned int const num_vertices;
};

auto
load_mesh(char const* path)
{
  Assimp::Importer importer;
  aiScene const* pscene = importer.ReadFile(path,
        aiProcess_SortByPType);
  assert(nullptr != pscene);

  assert(1 == pscene->mNumMeshes);
  aiMesh &mesh = *pscene->mMeshes[0];
  //assert(nullptr == pscene->mMeshes[1]);

  auto const num_vertices = mesh.mNumVertices;
  auto const num_floats = mesh.mNumFaces * 3 * 6;
  stlw::sized_buffer<float> floats{num_floats};

  for(auto i{0u}, k{0u}; i < mesh.mNumFaces; ++i)
  {
    auto const& f = mesh.mFaces[i];
    FOR(j, 3)
    {
      auto const& z = f.mIndices[j];
      aiVector3D const pos = mesh.mVertices[z];
      floats[k++] = pos.x;
      floats[k++] = pos.y;
      floats[k++] = pos.z;
      floats[k++] = 1.0f;

      //aiVector3D const normal = mesh.mNormals[z];
      //floats[k++] = normal.x;
      //floats[k++] = normal.y;
      //floats[k++] = normal.z;

      aiVector3D const uv = mesh.mTextureCoords[0][z];
      floats[k++] = uv.x;
      floats[k++] = uv.y;
    }
  }
  stlw::sized_buffer<int> indices{mesh.mNumFaces * 3};
  for (auto i{0u}, j{0u}; i < mesh.mNumFaces; ++i)
  {
    aiFace const& face = mesh.mFaces[i];
    assert(face.mNumIndices == 3);

    indices[j++] = face.mIndices[0];
    indices[j++] = face.mIndices[1];
    indices[j++] = face.mIndices[2];
  }
  return obj{MOVE(floats), MOVE(indices), num_vertices};
}

} // ns opengl
