#include "chunkArray.hpp"

static const Point3i nv[6] = {Vector3i{1,0,0},
                              Vector3i{0,1,0},
                              Vector3i{0,0,1},
                              Vector3i{-1,0,0},
                              Vector3i{0,-1,0},
                              Vector3i{0,0,-1}};


ChunkArray::ChunkArray(const Vector3i &dim, const Point3i &cPos)
  : mDim(dim), mData(dim[0]), mCenter(cPos)
{
  /*
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
          Chunk *chunk = &mData[x][y][z];
          chunk->setWorldPos(cPos + Point3i{x,y,z});
          chunk->setLoaded(false);
          
          for(int sx = -1; sx <= 1; sx++)
            for(int sy = -1; sy <= 1; sy++)
              for(int sz = -1; sz <= 1; sz++)
                {
                  if(!(sx == 0 && sy == 0 && sz == 0))
                    {
                      if(x+sx < 0 || y+sy < 0 || z+sz < 0 ||
                         x+sx == mDim[0] || y+sy == mDim[1] || z+sz == mDim[2])
                        { chunk->setNeighbor(getSide(sx, sy, sz), nullptr); }
                      else
                        { chunk->setNeighbor(getSide(sx, sy, sz), &mData[x+sx][y+sy][z+sz]); }
                    }
                }
        }
  */
}
ChunkArray::~ChunkArray()
{
  
}

Chunk*ChunkArray:: operator[](const Point3i &p)
{
  return &mData[p[0]][p[1]][p[2]];
}
const Chunk* ChunkArray::operator[](const Point3i &p) const
{
  return &mData[p[0]][p[1]][p[2]];
}
void ChunkArray::rotate(const Vector3i &amount)
{
  /*
  mData.rotate(amount[0]);
  for(int x = 0; x < mDim[0]; x++)
    {
      mData[x].rotate(amount[1]);
      for(int y = 0; y < mDim[1]; y++)
        {
          mData[x][y].rotate(amount[2]);
        }
    }
  
  for(int x = 0; x < mDim[0]; x++)
    for(int y = 0; y < mDim[1]; y++)
      for(int z = 0; z < mDim[2]; z++)
        {
          Chunk *chunk = &mData[x][y][z];
          Point3i cPos = chunk->pos() - mCenter;
          if(cPos[0] == 0 || cPos[0] == mDim[0]-1 ||
             cPos[1] == 0 || cPos[1] == mDim[1]-1 ||
             cPos[2] == 0 || cPos[2] == mDim[2]-1)
            {
              chunk->setLoaded(false);
              chunk->setWorldPos(chunk->pos() +
                                 Point3i{amount[0]*mDim[0], amount[1]*mDim[1], amount[2]*mDim[2]});
            }
              for(int sx = -1; sx <= 1; sx++)
                for(int sy = -1; sy <= 1; sy++)
                  for(int sz = -1; sz <= 1; sz++)
                    {
                      if(!(sx == 0 && sy == 0 && sz == 0))
                        {
                          if(x+sx < 0 || y+sy < 0 || z+sz < 0 ||
                             x+sx == mDim[0] || y+sy == mDim[1] || z+sz == mDim[2])
                            { chunk->setNeighbor(getSide(sx, sy, sz), nullptr); }
                          else
                            { chunk->setNeighbor(getSide(sx, sy, sz), &mData[x+sx][y+sy][z+sz]); }
                        }
                    }
        }
  
  mCenter += amount;
  */

  /*
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

          const int d1 = (d+1)%3;
          const int d2 = (d+2)%3;
  
          for(int x = uStart[0]; x < uEnd[0]; x++)
            for(int y = uStart[1]; y < uEnd[1]; y++)
              for(int z = uStart[2]; z < uEnd[2]; z++)
                {
                  const Point3i cp{x, y, z};
                  Chunk *chunk = at(cp);
                  //chunk->setActive(false);
                  chunk->setLoaded(false);
                  chunk->setMeshDirty(true);
                  chunk->setMeshUploaded(false);
                  chunk->setWorldPos(chunk->pos() + dPos);

                  Vector3i dp;
                  for(dp[d1] = -1; dp[d1] <= 1; dp[d1]++)
                    {
                      for(dp[d2] = -1; dp[d2] <= 1; dp[d2]++)
                        {
                          dp[d] = amount[d];
                          blockSide_t side = getSide(dp[0], dp[1], dp[2]);
                          Chunk* neighbor = at(cp + dp);
                          chunk->setNeighbor(side, nullptr);
                          //neighbor->setActive(false);
                          neighbor->setNeighbor(oppositeSide(side), nullptr);
                          //neighbor->setActive(true);
                          
                          dp[d] = -amount[d];
                          side = getSide(dp[0], dp[1], dp[2]);
                          neighbor = at(cp + dp);
                          chunk->setNeighbor(side, neighbor);
                          //neighbor->setActive(false);
                          neighbor->setNeighbor(oppositeSide(side), chunk);
                          //neighbor->setActive(true);
                        }
                    }
                  //chunk->setActive(true);
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
  */
  /*
  for(int x = 0; x < mDim[0]; x++)
    for(int y = 0; y < mDim[1]; y++)
      for(int z = 0; z < mDim[2]; z++)
        {
          Chunk *chunk = &mData[x][y][z];
          //chunk->resetNeighbors();
        }
  */
}
