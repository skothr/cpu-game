#ifndef MESHING_HPP
#define MESHING_HPP

#include "block.hpp"
#include "blockSides.hpp"
#include "hashing.hpp"
#include <unordered_map>
#include <mutex>

struct ActiveBlock
{
  block_t block = block_t::NONE;
  blockSide_t sides = blockSide_t::NONE;
};

struct ActiveRect
{
  block_t type;
  Point2i pos;
  Point2i size;
};

class Chunk;

class ChunkBounds
{
public:
  ChunkBounds();
  ChunkBounds(Chunk *chunk);
  ~ChunkBounds();

  bool calcBounds(Chunk *chunk);
  void simplifyGreedy();
  std::unordered_map<hash_t, ActiveBlock>& getBounds();
  int numFaces() const { return mNumFaces; }
  
  block_t getBlock(Chunk *chunk, const Point3i &wp, blockSide_t side);
  ActiveBlock* getBlock(hash_t hash);

  void lock() { mLock.lock(); }
  void unlock() { mLock.unlock(); }
  
private:
  std::mutex mLock;
  std::unordered_map<hash_t, ActiveBlock> mBounds;
  std::vector<ActiveRect> mSimplified;
  int mNumFaces = 0;
};



#endif // MESHING_HPP
