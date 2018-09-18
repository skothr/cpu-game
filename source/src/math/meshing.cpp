#include "meshing.hpp"
#include "world.hpp"
#include "chunk.hpp"

ChunkBounds::ChunkBounds()
{

}
ChunkBounds::ChunkBounds(Chunk *chunk)
{
  calcBounds(chunk);
}
ChunkBounds::~ChunkBounds()
{

}

block_t ChunkBounds::getBlock(Chunk *chunk, const Point3i &wp, blockSide_t side)
{
  Chunk *n = chunk->getNeighbor(side);
  if(n)
    { return n->getType(Chunk::blockPos(wp)); }
  else
    { return block_t::NONE; }
}

std::unordered_map<hash_t, ActiveBlock>& ChunkBounds::getBounds()
{
  return mBounds;
}

ActiveBlock* ChunkBounds::getBlock(hash_t hash)
{
  auto iter = mBounds.find(hash);
  if(iter == mBounds.end())
    { return nullptr; }
  else
    { return &iter->second; }
}
 
static const std::array<blockSide_t, 6> sides {{ blockSide_t::PX, blockSide_t::PY,
                                                    blockSide_t::PZ, blockSide_t::NX,
                                                    blockSide_t::NY, blockSide_t::NZ }};
static const std::array<Point3i, 6> sideDirections {{ Point3i{1,0,0}, Point3i{0,1,0},
                                                      Point3i{0,0,1}, Point3i{-1,0,0},
                                                      Point3i{0,-1,0}, Point3i{0,0,-1} }}; 
bool ChunkBounds::calcBounds(Chunk *chunk)
{
  lock();
  mBounds.clear();
  if(chunk->isEmpty())
    { return false; }
  
  // iterate over the chunk's blocks and compile all active faces.
  Point3i bp;
  Point3i cPos = chunk->pos();
  const Point3i minP{cPos[0]*Chunk::sizeX,
                     cPos[1]*Chunk::sizeY,
                     cPos[2]*Chunk::sizeZ };
  const Point3i maxP = minP + Chunk::size;
  int bi = 0;
  for(bp[1] = 0; bp[1] < Chunk::size[1]; bp[1]++)
    for(bp[2] = 0; bp[2] < Chunk::size[2]; bp[2]++)
      for(bp[0] = 0; bp[0] < Chunk::size[0]; bp[0]++, bi++)
	{
          block_t bt = *chunk->at(bi);
          if(isSimpleBlock(bt))// != block_t::NONE)
            { // block is not empty
              blockSide_t activeSides = blockSide_t::NONE;
              bool blockActive = false;
              Point3i p = minP + bp;
              for(int i = 0; i < 6; i++)
                {
                  Point3i np = bp + sideDirections[i];
                  if(!isSimpleBlock(((Chunk::chunkEdge(bp) & sides[i]) != blockSide_t::NONE) ?
                                    getBlock(chunk, np, sides[i]) : chunk->getType(np)))
                    { // block on side i is empty (face is active)
                      blockActive = true;
                      activeSides |= sides[i];
                      mNumFaces++;
                    }
                }
              
              if(blockActive)
                { mBounds.emplace(Hash::hash(bp), ActiveBlock{bt, activeSides}); }
            }
        }

  unlock();
  return mBounds.size() > 0;
}

bool lessThan(const ActiveRect &r1, const ActiveRect &r2)
{
  if(r1.pos[1] != r2.pos[1]) return r1.pos[1] < r2.pos[1];
  if(r1.pos[0] != r2.pos[0]) return r1.pos[0] < r2.pos[0];
  if(r1.size[0] != r2.size[0]) return r1.size[0] > r2.size[0];
  return r1.size[1] >= r2.size[1];
}

ActiveRect blockToRect(block_t type, const Point3i &bp, blockSide_t side)
{
  switch(side)
    {
    case blockSide_t::PX:
      return {type, Point2i{bp[1], bp[2]}, Vector2i{1, 1}};
    case blockSide_t::PY:
      return {type, Point2i{bp[0], bp[2]}, Vector2i{1, 1}};
    case blockSide_t::PZ:
      return {type, Point2i{bp[0], bp[1]}, Vector2i{1, 1}};
    case blockSide_t::NX:
      return {type, Point2i{bp[1], bp[2]}, Vector2i{1, 1}};
    case blockSide_t::NY:
      return {type, Point2i{bp[0], bp[2]}, Vector2i{1, 1}};
    case blockSide_t::NZ:
      return {type, Point2i{bp[0], bp[1]}, Vector2i{1, 1}};
    };
}

void ChunkBounds::simplifyGreedy()
{
  std::array<std::unordered_map<block_t, std::vector<ActiveRect>>, 6> sideRects;

  for(int s = 0; s < 6; s++)
    {
      for(int i = 1; i < (int)block_t::COUNT; i++)
        {
          sideRects[s].emplace((block_t)i, std::vector<ActiveRect>{});
        }
    }
  
  for(auto iter : mBounds)
    {
      Point3i bp =  Hash::unhash(iter.first);
      for(int i = 0; i < 6; i++)
        {
          if((bool)(iter.second.sides & sides[i]))
            {
              sideRects[i][iter.second.block].push_back(blockToRect(iter.second.block, bp, sides[i]));
            }
        }
    }
  
  // combine active rectangles
  for(int i = 0; i < 6; i++)
    {
      for(auto &iter : sideRects[i])
        {
          std::vector<ActiveRect> &rects = iter.second;
          std::sort(rects.begin(),rects.end(),
                    [=](const ActiveRect &r1, const ActiveRect &r2)
                    { return lessThan(r1, r2); });
          /*
          int ri = 0;
          while(ri < rects.size())
            {
              
            }
          */
        }
    }
  
}
