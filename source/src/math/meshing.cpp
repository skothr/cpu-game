#include "meshing.hpp"
#include "hashing.hpp"
#include "world.hpp"

OuterShell::OuterShell()
{

}
OuterShell::OuterShell(Chunk *chunk, std::unordered_map<blockSide_t, Chunk*> &neighbors)
{
  calcShell(chunk, neighbors);
}
OuterShell::~OuterShell()
{

}


block_t OuterShell::getBlock(const Point3i &wp, blockSide_t side,
                             std::unordered_map<blockSide_t, Chunk*> &neighbors )
{
  auto iter = neighbors.find(side);
  if(iter != neighbors.end() && iter->second)
    { return iter->second->getType(Chunk::blockPos(wp)); }
  else
    { return block_t::NONE; }
}

std::unordered_map<int32_t, ActiveBlock>& OuterShell::getShell()
{
  return mShell;
}

ActiveBlock* OuterShell::getBlock(int32_t hash)
{
  auto iter = mShell.find(hash);
  if(iter == mShell.end())
    { return nullptr; }
  else
    { return &iter->second; }
}
  
bool OuterShell::calcShell(Chunk *chunk, std::unordered_map<blockSide_t, Chunk*> &neighbors)
{
  static const std::array<blockSide_t, 6> sides {{ blockSide_t::PX, blockSide_t::PY,
                                                   blockSide_t::PZ, blockSide_t::NX,
                                                   blockSide_t::NY, blockSide_t::NZ }};
  static const std::array<Point3i, 6> sideDirections {{ Point3i{1,0,0}, Point3i{0,1,0},
                                                        Point3i{0,0,1}, Point3i{-1,0,0},
                                                        Point3i{0,-1,0}, Point3i{0,0,-1} }};
  mShell.clear();
  if(chunk->isEmpty())
    { return false; }
  
  // iterate over the chunk's blocks and compile all active faces.
  Point3i bp;
  Point3i cPos = chunk->pos();
  const Point3i minP{cPos[0]*Chunk::sizeX,
                     cPos[1]*Chunk::sizeY,
                     cPos[2]*Chunk::sizeZ };
  const Point3i maxP = minP + Chunk::size;
  for(bp[0] = 0; bp[0] < Chunk::size[0]; bp[0]++)
    for(bp[1] = 0; bp[1] < Chunk::size[1]; bp[1]++)
      for(bp[2] = 0; bp[2] < Chunk::size[2]; bp[2]++)
	{
	  block_t bt = chunk->getType(bp);
          if(bt != block_t::NONE)
            { // block is not empty
              blockSide_t activeSides = blockSide_t::NONE;
              bool blockActive = false;
              Point3i p = minP + bp;
              for(int i = 0; i < 6; i++)
                {
                  if((bool)(chunk->chunkEdge(bp) & sides[i]) ?
                     getBlock(bp + sideDirections[i], sides[i], neighbors) == block_t::NONE :
                     (chunk->getType(bp + sideDirections[i]) == block_t::NONE))
                    { // block on side i is empty (face is active)
                      blockActive = true;
                      activeSides |= sides[i];
                      mNumFaces++;
                    }
                }
              
              if(blockActive)
                { mShell.emplace(Hash::hash(bp), ActiveBlock{bt, activeSides}); }
            }
        }
  
  return mShell.size() > 0;
}
