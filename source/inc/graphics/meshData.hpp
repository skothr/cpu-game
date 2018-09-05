#ifndef MESH_DATA_HPP
#define MESH_DATA_HPP

#include <vector>
#include "vertex.hpp"

class MeshData
{
public:
  MeshData(bool doubleBuffer = false);
  ~MeshData();

  bool empty() const;
  void swap();

  const std::vector<cSimpleVertex>& vertices() const { return mVertices[mActive]; }
  const std::vector<unsigned int>& indices() const { return mIndices[mActive]; }
  std::vector<cSimpleVertex>& vertices() { return mVertices[mActive]; }
  std::vector<unsigned int>& indices() { return mIndices[mActive]; }

private:
  std::vector<cSimpleVertex> mVertices[2];
  std::vector<unsigned int> mIndices[2];
  int mActive = 0;
  bool mDoubleBuffer;
};


#endif // MESH_DATA_HPP
