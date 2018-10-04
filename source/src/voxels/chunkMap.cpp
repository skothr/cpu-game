#include "chunkMap.hpp"

#include "chunkLoader.hpp"
#include "camera.hpp"
#include "world.hpp"
#include "pointMath.hpp"

#include <chrono>


ChunkMap::ChunkMap()
{
  
}

ChunkMap::~ChunkMap()
{
  clear();
  std::lock_guard<std::mutex> lock(mChunkLock);
  while(mUnusedChunks.size() > 0)
    {
      delete mUnusedChunks.front();
      mUnusedChunks.pop();
    }
}

void ChunkMap::setLoader(ChunkLoader *loader)
{
  mLoader = loader;
}
void ChunkMap::setCamera(Camera *camera)
{
  mCamera = camera;
}

void ChunkMap::lock()
{
  mChunkLock.lock();
}
void ChunkMap::unlock()
{
  mChunkLock.unlock();
}
std::unordered_map<hash_t, ChunkPtr>& ChunkMap::getChunks()
{
  return mChunks;
}

void ChunkMap::load(const Point3i &cp)
{
  //mLoadOrder.push_back(cp);
  Vector3f camPos = mCamera->getPos();
  mCamPos = World::chunkPos(Vector3i{camPos[0], camPos[1], camPos[2]});
  Vector3f diff = mCamPos - cp;
  //int newPriority = chunkPriority(cp);
  int newPriority = diff.dot(diff);
  std::lock_guard<std::mutex> lock(mLoadLock);
  if(newPriority < 0 || mLoadOrder.size() == 0)
    {
      mLoadOrder.push_back(cp);
      return;
    }
  else
    {
      for(auto iter = mLoadOrder.begin(); iter != mLoadOrder.end(); ++iter)
        {
          diff = mCamPos - (*iter);
          int priority = diff.dot(diff);
          if(newPriority < priority)
            {
              mLoadOrder.insert(iter, cp);
              break;
            }
        }
    }
}
void ChunkMap::unload(const Point3i &cp)
{
  //std::lock_guard<std::mutex> lock(mChunkLock);
  unloadChunk(Hash::hash(cp));
}

void ChunkMap::clear()
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  for(auto iter : mChunks)
    { mUnusedChunks.push(iter.second); }
  mChunks.clear();
  mLoadingChunks.clear();
  mChunkNeighbors.clear();

  mNumLoading = 0;
  mNumLoaded = 0;
}


Point3i ChunkMap::getPos() const
{ return (mCamera ? World::chunkPos(mCamera->getPos()) : Point3i()); }

TreeNode ChunkMap::getTree()
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  return mVisibleTree;
}

bool ChunkMap::isLoading(const Point3i &cp)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  return (mLoadingChunks.count(Hash::hash(cp)) > 0);
}
bool ChunkMap::isLoaded(const Point3i &cp)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  return (mChunks.find(Hash::hash(cp)) != mChunks.end());
}
bool ChunkMap::isReady(const Point3i &cp)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
      auto nIter = mChunkNeighbors.find(Hash::hash(cp));
      if(nIter != mChunkNeighbors.end() && allSides(nIter->second))
        {
          //iter->second->setReady(true);
          return true;
        }
  
  // auto iter = mChunks.find(Hash::hash(cp));
  // if(iter != mChunks.end())
  //   {
  //     if(iter->second->isReady())
  //       { return true; }

      
  //     auto nIter = mChunkNeighbors.find(Hash::hash(cp));
  //     if(allSides(nIter->second))
  //       {
  //         iter->second->setReady(true);
  //         return true;
  //       }
  //   }
  return false;
}
bool ChunkMap::isUnloaded(const Point3i &cp)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  return (mUnloadedChunks.count(Hash::hash(cp)) > 0);
}
void ChunkMap::clearUnloaded()
{ mUnloadedChunks.clear(); }

sideFlag_t ChunkMap::getNeighbors(hash_t hash)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  auto iter = mChunkNeighbors.find(hash);
  if(iter != mChunkNeighbors.end())
    {
      return iter->second;
    }
  else
    {
      return 0x00;
    }
}
int ChunkMap::numLoading() const
{ return mNumLoading; }
int ChunkMap::numLoaded() const
{ return mNumLoaded; }

void ChunkMap::updateAdjacent(const Point3i &cp, blockSide_t edges)
{
  const Point3i np = cp + sideDirection(edges);
  const Point3i minP{std::min(cp[0], np[0]),
                     std::min(cp[1], np[1]),
                     std::min(cp[2], np[2]) };
  const Point3i maxP{std::max(cp[0], np[0]),
                     std::max(cp[1], np[1]),
                     std::max(cp[2], np[2]) };
  Point3i p;
  std::lock_guard<std::mutex> lock(mChunkLock);
  for(p[0] = minP[0]; p[0] <= maxP[0]; p[0]++)
    for(p[1] = minP[1]; p[1] <= maxP[1]; p[1]++)
      for(p[2] = minP[2]; p[2] <= maxP[2]; p[2]++)
        {
          auto iter = mChunks.find(Hash::hash(p));
          if(iter != mChunks.end())
            { iter->second->setDirty(true); }
        }
}


void ChunkMap::chunkFinishedLoading(Chunk *chunk)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  const hash_t hash = Hash::hash(chunk->pos());
  {
    if(mLoadingChunks.count(hash) == 0)
      {
        LOGW("Chunk was unloaded before loading finished!");
        mUnusedChunks.push(chunk);
        return;
      }
    else
      {
        mLoadingChunks.erase(hash);
      }
  }
      
  // check which neighbor chunks are loaded
  sideFlag_t neighbors = 0;
  for(auto side : gBlockSides)
    {
      hash_t nHash = chunk->neighborHash(side);

      ChunkPtr nChunk = nullptr;
      {
        auto nIter = mChunks.find(nHash);
        if(nIter != mChunks.end())
          { nChunk = nIter->second; }
      }
      if(nChunk)
        {
          chunk->setNeighbor(side, nChunk);
          neighbors |= sideFlag(side);
          nChunk->setNeighbor(oppositeSide(side), chunk);
          
          //std::lock_guard<std::mutex> lock(mNeighborLock);
          auto nnIter = mChunkNeighbors.find(nHash);
          if(nnIter != mChunkNeighbors.end())
            {
              bool update = !allSides(nnIter->second);
              nnIter->second |= sideFlag(oppositeSide(side));
            
              if(allSides(nnIter->second))
                {
                  nChunk->setReady(true);
                  //if(update)
                    { nChunk->setDirty(true); }
                }
            }
        }
    }

  chunk->updateConnected();
  chunk->setDirty(true);
      
  if(allSides(neighbors))
    { chunk->setReady(true); }
      
  //std::lock_guard<std::mutex> lock(mChunkLock);
  
  // add chunk
  mChunks.emplace(hash, chunk);
  mChunkNeighbors.emplace(hash, neighbors);
  mNumLoading--;
  mNumLoaded++;
}

ChunkPtr ChunkMap::operator[](const Point3i &cp)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  const hash_t hash = Hash::hash(cp);
  auto iter = mChunks.find(hash);
  if(iter != mChunks.end())
    { return iter->second; }
  else
    { return nullptr; }
}
ChunkPtr ChunkMap::operator[](hash_t hash)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  auto iter = mChunks.find(hash);
  if(iter != mChunks.end())
    { return iter->second; }
  else
    { return nullptr; }
}

void ChunkMap::loadChunk(hash_t hash)
{
  std::lock_guard<std::mutex> lock(mChunkLock);
  {
    auto iter = mChunks.find(hash);
    if(iter != mChunks.end() || mLoadingChunks.count(hash) > 0)
      { return; }
  }
  const Point3i p = Hash::unhash(hash);
  ChunkPtr chunk;
  if(mUnusedChunks.size() > 0)
    {
      chunk = mUnusedChunks.front();
      mUnusedChunks.pop();
      chunk->setWorldPos(p);
      chunk->setReady(false);
      chunk->setDirty(false);
      chunk->setPriority(false);
      chunk->setUnloaded(false);
    }
  else
    {
      chunk = new Chunk(p);
    }
  
  mLoadingChunks.insert(hash);
  mNumLoading++;
  mLoader->load(chunk);
}

void ChunkMap::unloadChunk(hash_t hash)
{
  Point3i p = Hash::unhash(hash);
  {
    std::lock_guard<std::mutex> lock(mLoadLock);
    mLoadOrder.remove(p);
  }

  std::lock_guard<std::mutex> lock(mChunkLock);
  ChunkPtr chunk = nullptr;
  {  
    //mLoadingChunks.erase(hash);
    auto iter = mChunks.find(hash);
    if(iter != mChunks.end())
      { chunk = iter->second; }
  }

  if(chunk)
    {
      mChunkNeighbors.erase(hash);
      
      // unready neighbors
      for(auto side : gBlockSides)
        {
          //std::lock_guard<std::mutex> lock(mNeighborLock);
          const hash_t nHash = chunk->neighborHash(oppositeSide(side));
          auto nIter = mChunkNeighbors.find(nHash);
          if(nIter != mChunkNeighbors.end())
            {
              if(chunk->getNeighbor(side))
                { chunk->getNeighbor(side)->unsetNeighbor(side); }
              bool update = allSides(nIter->second);
              nIter->second &= ~sideFlag(side);
              if(update)
                {
                  //std::lock_guard<std::mutex> lock(mChunkLock);
                  //mUnloadedChunks.insert(nHash);
                  auto ncIter = mChunks.find(nHash);
                  ncIter->second->setDirty(true);
                  ncIter->second->setReady(false);
                  ncIter->second->setUnloaded(true);
                }
            }
        }
      
      // remove chunk
      //mUnloadedChunks.insert(hash);
      chunk->setReady(false);
      chunk->setUnloaded(true);
      mUnusedChunks.push(chunk);
      mChunks.erase(hash);
      mNumLoaded--;
    }
  else
    {
      if(mLoadingChunks.count(hash) > 0)
        { // stop loading
          {
            mLoadingChunks.erase(hash);
            mNumLoading--;
          }
          {
            std::lock_guard<std::mutex> lock(mLoadLock);
            mLoadOrder.remove(p);
          }
        }
      else
        {
          LOGW("Tried to unload chunk that wasn't loaded!");
        }
    }
}

static const std::vector<blockSide_t> stepSides { blockSide_t::PX,
                                                   blockSide_t::PY,
                                                   blockSide_t::PZ,
                                                   blockSide_t::NX,
                                                   blockSide_t::NY,
                                                   blockSide_t::NZ };
static const std::vector<Vector3f> stepDirs { sideDirection(stepSides[0]),
                                              sideDirection(stepSides[1]),
                                              sideDirection(stepSides[2]),
                                              sideDirection(stepSides[3]),
                                              sideDirection(stepSides[4]),
                                              sideDirection(stepSides[5]) };

std::vector<hash_t> ChunkMap::updateTree()
{
  std::vector<hash_t> loadOrder;
  /*
  Point3f camPos = mCamera->getPos();
  mCamPos = World::chunkPos(Point3i{camPos[0], camPos[1], camPos[2]});
  mChunkTree.pos = mCamPos;

  std::unordered_set<hash_t> traversed;
  std::queue<std::pair<OrderNode, blockSide_t>> steps; // (index of mRenderOrder, and direction)

  const hash_t cHash = Hash::hash(mCamPos);
  ChunkPtr chunk = nullptr;
  {
    std::lock_guard<std::mutex> lock(mChunkLock);
    auto iter = mChunks.find(cHash);
    if(iter == mChunks.end())
      { return {}; }
    else
      { chunk = iter->second; }
  }

  std::unordered_map<hash_t, int> treeOrder;
  TreeNode chunkTree;
  {
    std::lock_guard<std::mutex> lock(mTreeLock);
    //mTreeOrder.clear();
    treeOrder.emplace(cHash, 0);
    chunkTree = TreeNode(chunk, nullptr, 0);
    steps.push({{cHash, &chunkTree}, blockSide_t::NONE});
  }
      
  
  while(steps.size() > 0)
    {
      const OrderNode order = steps.front().first;
      const blockSide_t prevSide = steps.front().second;
      steps.pop();
      
      const hash_t hash = order.hash;

      ChunkPtr chunk = nullptr;
      {
        std::lock_guard<std::mutex> lock(mChunkLock);
        auto cIter = mChunks.find(hash);
        if(cIter != mChunks.end())
          { chunk = cIter->second; }
      }
      
      if(chunk)
        {
          // propogate to each unvisited side
          for(int i = 0; i < 6; i++)
            {
              if(stepSides[i] != prevSide &&
                 ((prevSide == blockSide_t::NONE && chunk->sideOpen(stepSides[i])) ||
                  (chunk->edgesConnected(prevSide, stepSides[i])) ))
                {
                  const Point3i nextPos = chunk->pos() + stepDirs[i];
                  const hash_t nextHash = Hash::hash(nextPos);
                  if(traversed.count(nextHash) == 0)
                    { //  unvisited chunk
                      traversed.insert(nextHash);
                      loadOrder.push_back(nextHash);

                      ChunkPtr nextChunk = nullptr;
                      {
                        std::lock_guard<std::mutex> lock(mChunkLock);
                        auto cIter2 = mChunks.find(nextHash);
                        if(cIter2 != mChunks.end())
                          { nextChunk = cIter2->second; }
                        else
                          {
                            treeOrder.emplace(nextHash, order.node->depth+1);
                            order.node->children.back().pos = nextPos;
                            order.node->children.back().hash = nextHash;
                          }
                      }

                      if(order.node)
                        {
                          order.node->children.emplace_back(nextChunk,
                                                            order.node,
                                                            order.node->depth + 1);
                          steps.push({{nextHash, &order.node->children.back()},
                                      oppositeSide(stepSides[i]) });
                        }
                    }
                }
            }
        }
*/
  return loadOrder;
}

std::vector<hash_t> ChunkMap::getVisible(Camera *cam)
{
  std::vector<hash_t> visible;
  if(!cam)
    { // add all loaded chunks
      std::lock_guard<std::mutex> lock(mChunkLock);
      visible.reserve(mChunks.size());
      for(auto &iter : mChunks)
        { visible.push_back(iter.first); }
      
      return visible;
    }
  Point3f camPos = cam->getPos();
  Point3i iCamPos = World::chunkPos(Point3i{camPos[0], camPos[1], camPos[2]});
  mVisibleTree.pos = iCamPos;

  const hash_t cHash = Hash::hash(iCamPos);
  visible.push_back(cHash);
  
  std::unordered_set<hash_t> traversed;
  std::queue<std::pair<OrderNode, blockSide_t>> steps; // (index of mRenderOrder, and direction)

  ChunkPtr chunk = nullptr;
  {
    std::lock_guard<std::mutex> lock(mChunkLock);
    auto iter = mChunks.find(cHash);
    if(iter != mChunks.end())
      {
        chunk = iter->second;
      }
  }
  
  // TODO:
  //  - stop propogating if already behind other chunks
  //    (see minecraft cave culling example)
  //     **like reverse of world raycast, with more of a range**
  //  - more filters
  
  mVisibleTree = TreeNode(chunk, nullptr, 0, true);
  steps.push({{cHash, &mVisibleTree}, blockSide_t::NONE});
  
  while(steps.size() > 0)
    {
      const OrderNode order = steps.front().first;
      const blockSide_t prevSide = steps.front().second;
      steps.pop();
      
      const hash_t hash = order.hash;

      ChunkPtr chunk = nullptr;
      {
        std::lock_guard<std::mutex> lock(mChunkLock);
        auto cIter = mChunks.find(hash);
        if(cIter != mChunks.end())
          { chunk = cIter->second; }
      }

      Point3i cp;// = Hash::unhash(hash);
      if(chunk)
        { cp = chunk->pos(); }
      
      if(!chunk || cam->cubeInFrustum(Vector3f(cp)*Chunk::size, Vector3f(Chunk::size)) )
        {
          // propogate to each unvisited side
          for(int i = 0; i < 6; i++)
            {
              if(stepSides[i] != prevSide && (!chunk || prevSide == blockSide_t::NONE ||
                                              chunk->edgesConnected(prevSide, stepSides[i]) ))
                {
                  const Point3i nextPos = cp + stepDirs[i];
                  const hash_t nextHash = Hash::hash(nextPos);
                  if(traversed.count(nextHash) == 0)
                    { //  unvisited chunk
                      traversed.insert(nextHash);
                      visible.push_back(nextHash);

                      ChunkPtr nextChunk = nullptr;
                      {
                        std::lock_guard<std::mutex> lock(mChunkLock);
                        auto cIter2 = mChunks.find(nextHash);
                        if(cIter2 != mChunks.end())
                          { nextChunk = cIter2->second; }
                      }
                      if(order.node)
                        {
                          order.node->children.emplace_back(nextChunk,
                                                            order.node,
                                                            order.node->depth + 1);
                          steps.push({{nextHash, &order.node->children.back()},
                                      oppositeSide(stepSides[i]) });
                        }
                    }
                }
            }
        }
    }
  
  return visible;
}

std::vector<hash_t> ChunkMap::unloadOutside(const Point3i minChunk, const Point3i maxChunk)
{
  std::vector<hash_t> unloadChunks;
  {
    std::lock_guard<std::mutex> lock(mChunkLock);
    for(auto iter : mChunks)
      {
        if(!pointInRange(iter.second->pos(), minChunk, maxChunk))
          { // chunk out of range -- unload
            //LOGD("Unloading chunk (out of range) --> %d", (int)iter.first);
            unloadChunks.push_back(iter.first);
          }
      }
  }
  for(auto hash : unloadChunks)
    {
      //std::lock_guard<std::mutex> lock(mChunkLock);
      unloadChunk(hash);
    }
  
  //unloadChunks.insert(unloadChunks.end(), mUnloadedChunks.begin(), mUnloadedChunks.end());
  //mUnloadedChunks.clear();
  return unloadChunks;
}

void ChunkMap::update()
{
  //auto startT = std::chrono::high_resolution_clock::now();
  //updateTree();
  //std::vector<hash_t> order = updateTree();
  //LOGD("Tree update time: %fs", (std::chrono::high_resolution_clock::now() - startT).count());

  //startT = std::chrono::high_resolution_clock::now();
  //mLoadOrder.sort([this](const Point3i &p1, const Point3i &p2)
  //                { return chunkPriority(p1) > chunkPriority(p2); });
  while(mLoadOrder.size() > 0 && mNumLoading < 256)
    {
      std::lock_guard<std::mutex> lock(mLoadLock);
      //{
        //std::lock_guard<std::mutex> lock(mLoadLock);
        Point3i cp = mLoadOrder.front();
        mLoadOrder.pop_front();
        //}
      //std::lock_guard<std::mutex> lock(mChunkLock);
      loadChunk(Hash::hash(cp));
    }
  //LOGD("Load update time: %fs", (std::chrono::high_resolution_clock::now() - startT).count());
}

int ChunkMap::chunkPriority(const Point3i &cp)
{
  /*
  //std::lock_guard<std::mutex> lock(mChunkLock);
  Point3f camPos = mCamera->getPos();
  mCamPos = World::chunkPos(Point3i{camPos[0], camPos[1], camPos[2]});

  hash_t hash = Hash::hash(cp);

  Vector3i diff = (mCamPos - cp).abs();
  int maxDiff = diff.max();
  
  if(maxDiff < 3)
    { return maxDiff; }
  else
    {
      // auto tIter = mTreeOrder.find(hash);
      // if(tIter != mTreeOrder.end())
      //   { return tIter->second; }
      // else
        {
          Vector3i diff = mCamPos - cp;
          //return 1000 + diff.dot(diff);
          return diff.dot(diff);
        }
    }
  */
  return 0;
}
