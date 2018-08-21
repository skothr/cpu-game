#include "chunkArray.hpp"

static const Point3i nv[6] = {Vector3i{1,0,0},
                              Vector3i{0,1,0},
                              Vector3i{0,0,1},
                              Vector3i{-1,0,0},
                              Vector3i{0,-1,0},
                              Vector3i{0,0,-1}};


cChunkArray::cChunkArray(const Vector3i &dim, const Point3i &cPos)
  : mDim(dim), mData(dim[0])
{
  for(int x = 0; x < mDim[0]; x++)
    {
      mData[x].resize(mDim[1]);
      for(int y = 0; y < mDim[1]; y++)
        {
          mData[x][y].resize(mDim[2]);
        }
    }
  
  for(int x = 0; x < mDim[0]; x++)
    for(int y = 0; y < mDim[1]; y++)
      for(int z = 0; z < mDim[2]; z++)
        {
          cChunk *chunk = &mData[x][y][z];
          chunk->setWorldPos(cPos + Point3i{x,y,z});
          chunk->setLoaded(false);
          chunk->setMeshUploaded(false);
          chunk->setMeshDirty(true);
          for(int sx = -1; sx <= 1; sx++)
            for(int sy = -1; sy <= 1; sy++)
              for(int sz = -1; sz <= 1; sz++)
                {
                  if(sx != 0 || sy != 0 || sz != 0)
                    {
                      cChunk* neighbor = &mData[x+sx][y+sy][z+sz];
                      blockSide_t side = getSide(sx, sy, sz);
                      chunk->setNeighbor(side, neighbor);
                      neighbor->setNeighbor(oppositeSide(side), chunk);
                    }
                }
        }
}
cChunkArray::~cChunkArray()
{
  
}

cChunk*cChunkArray:: operator[](const Point3i &p)
{
  return &mData[p[0]][p[1]][p[2]];
}
const cChunk* cChunkArray::operator[](const Point3i &p) const
{
  return &mData[p[0]][p[1]][p[2]];
}
void cChunkArray::rotate(const Vector3i &amount)
{
  Vector3i change{std::abs(amount[0]), std::abs(amount[1]), std::abs(amount[2])};
  Point3i uStart;
  Point3i uEnd;
  Vector3i dPos;

  for(int d = 0; d < 3; d++)
    {
      if(amount[d] != 0)
        {
          Point3i dAmount;
          dAmount[d] = amount[d];
          for(int i = 0; i < 3; i++)
            {
              if(dAmount[i] > 0)
                {
                  uStart[i] = 0;
                  uEnd[i] = change[i];
                  dPos[i] = mDim[i];
                }
              else if(dAmount[i] < 0)
                {
                  uStart[i] = mDim[i]-change[i];
                  uEnd[i] = mDim[i];
                  dPos[i] = -mDim[i];
                }
              else
                {
                  uStart[i] = 0;
                  uEnd[i] = mDim[i];
                  dPos[i] = 0;
                } 
            }
          //std::cout << "Chunk array rotating --> start: " << uStart << ", end: " << uEnd << ", dpos: " << dPos << "\n";
          //std::cout << "   amount:  " << amount << ", change: " << change << "\n";

  
          for(int x = uStart[0]; x < uEnd[0]; x++)
            for(int y = uStart[1]; y < uEnd[1]; y++)
              for(int z = uStart[2]; z < uEnd[2]; z++)
                {
                  cChunk &chunk = mData[x][y][z];
                  //LOGD("MOVING CHUNK AT: %d, %d, %d", chunk.pos()[0], chunk.pos()[1], chunk.pos()[2]);
                  //LOGD("             TO: %d, %d, %d", chunk.pos()[0]+dPos[0], chunk.pos()[1]+dPos[1], chunk.pos()[2]+dPos[2]);
                  chunk.setWorldPos(chunk.pos() + dPos);
                  chunk.setLoaded(false);
                  chunk.setMeshDirty(true);
                  chunk.setMeshUploaded(false);

                  for(int sx = -1; sx <= 1; sx++)
                    for(int sy = -1; sy <= 1; sy++)
                      for(int sz = -1; sz <= 1; sz++)
                        {
                          if(sx != 0 || sy != 0 || sz != 0)
                            {
                              cChunk* neighbor = &mData[x+sx][y+sy][z+sz];
                              neighbor->setMeshDirty(true);
                              neighbor->setMeshUploaded(false);
                            }
                        }
                  //chunk.updateBlocks();
                  
                }
        }
    }
  mData.rotate(amount[0]);
  for(int x = 0; x < mDim[0]; x++)
    {
      mData[x].rotate(amount[1]);
      for(int y = 0; y < mDim[1]; y++)
        {
          mData[x][y].rotate(amount[2]);
        }
    }
}
