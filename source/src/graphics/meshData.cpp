#include "meshData.hpp"


MeshData::MeshData()
{ }

bool MeshData::empty() const
{ return (indices.size() == 0 || vertices.size() == 0); }
