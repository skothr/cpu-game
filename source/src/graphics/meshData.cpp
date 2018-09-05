#include "meshData.hpp"


MeshData::MeshData(bool doubleBuffer)
  : mDoubleBuffer(doubleBuffer)
{ }

MeshData::~MeshData()
{ }

bool MeshData::empty() const
{ return (indices().size() == 0 || vertices().size() == 0); }

void MeshData::swap()
{
  if(mDoubleBuffer)
    { mActive = (mActive + 1) % 2; }
  vertices().clear();
  indices().clear();
}
