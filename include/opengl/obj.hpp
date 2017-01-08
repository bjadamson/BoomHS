#pragma once
#include <opengl/types.hpp>
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <cassert>

namespace opengl
{

struct obj
{
  stlw::sized_buffer<float> buffer;
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
  if (pscene == nullptr) {
    assert(15 == 35);
  }

  aiMesh *pmesh = pscene->mMeshes[0];
  assert(nullptr != pmesh);

  auto &mesh = *pmesh;
  int const num_v = mesh.mNumFaces * 3 * 3;
  int const num_n = mesh.mNumFaces * 3 * 3;
  //int const num_uv = mesh.mNumFaces * 3 * 2;
  std::size_t const num_floats = num_v + num_n;// + num_uv;

  stlw::sized_buffer<float> floats{num_floats};
  for(auto i{0u};i < mesh.mNumFaces; i++)
  {
    aiFace const& face = mesh.mFaces[i];
    for(int k{0}, j{0}; j < 3; ++j)
    {
      auto const& face_indice = face.mIndices[j];
      aiVector3D const pos = mesh.mVertices[face_indice];
      floats[k++] = pos.x;
      floats[k++] = pos.y;
      floats[k++] = pos.z;

      aiVector3D const normal = mesh.mNormals[face_indice];
      floats[k++] = normal.x;
      floats[k++] = normal.y;
      floats[k++] = normal.z;

      //aiVector3D const uv = mesh.mTextureCoords[0][face_indice];
      //floats[k++] = uv.x;
      //floats[k++] = uv.y;
    }
  }
  return obj{MOVE(floats)};
}

} // ns opengl
