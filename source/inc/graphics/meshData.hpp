#ifndef MESH_DATA_HPP
#define MESH_DATA_HPP

#include <vector>
#include "vertex.hpp"

class MeshData
{
public:
  MeshData();

  bool empty() const;

  std::vector<cSimpleVertex> vertices;
  std::vector<unsigned int> indices;
};


#endif // MESH_DATA_HPP
