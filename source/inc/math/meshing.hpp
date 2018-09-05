#ifndef MESHING_HPP
#define MESHING_HPP


#include "block.hpp"
#include "chunk.hpp"
#include "blockSides.hpp"
#include <unordered_map>
#include <mutex>

struct ActiveBlock
{
  block_t block = block_t::NONE;
  blockSide_t sides = blockSide_t::NONE;
};

class OuterShell
{
public:
  OuterShell();
  OuterShell(Chunk *chunk, std::unordered_map<blockSide_t, Chunk*> &neighbors);
  ~OuterShell();

  bool calcShell(Chunk *chunk, std::unordered_map<blockSide_t, Chunk*> &neighbors);
  block_t getBlock(const Point3i &wp, blockSide_t side, std::unordered_map<blockSide_t, Chunk*> &neighbors);
  std::unordered_map<int32_t, ActiveBlock>& getShell();
  ActiveBlock* getBlock(int32_t hash);
  int numFaces() const { return mNumFaces; }

  void lock() { mLock.lock(); }
  void unlock() { mLock.unlock(); }
  
private:
  std::mutex mLock;
  std::unordered_map<int32_t, ActiveBlock> mShell;
  int mNumFaces = 0;
};



#endif // MESHING_HPP
