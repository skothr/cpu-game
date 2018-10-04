#ifndef CHUNK_MAP_HPP
#define CHUNK_MAP_HPP

#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <vector>
#include <deque>
#include <mutex>
#include <atomic>

#include "chunk.hpp"
#include "hashing.hpp"
#include "vector.hpp"
#include "traversalTree.hpp"

class ChunkLoader;
class Camera;

class ChunkMap
{
  friend class World;
public:
  ChunkMap();
  ~ChunkMap();

  void setLoader(ChunkLoader *loader);
  void setCamera(Camera *camera);
  Point3i getPos() const;
  void setRadius(const Vector3i &rad) { mRadius = rad; }
  Vector3i getRadius() const { return mRadius; }

  void load(const Point3i &cp);
  void unload(const Point3i &cp);
  void clear();
  void update();
  
  struct OrderNode
  {
    //ChunkPtr chunk;
    hash_t hash;
    TreeNode *node;
  };
  
  //void updateOrder();
  std::vector<hash_t> updateTree();
  //std::vector<OrderLine>& getOrder();
  TreeNode getTree();
  std::vector<hash_t> getVisible(Camera *cam);

  bool isLoading(const Point3i &cp); // chunk is loading                  
  bool isLoaded(const Point3i &cp);  // chunk and all neighbors are loaded
  bool isReady(const Point3i &cp);   // chunk is done loading
  bool isUnloaded(const Point3i &cp); // chunk has been unloaded
  void clearUnloaded();
  sideFlag_t getNeighbors(hash_t hash);
  int numLoading() const;
  int numLoaded() const;

  void updateAdjacent(const Point3i &cp, blockSide_t edges);
  
  std::vector<hash_t> unloadOutside(const Point3i minChunk, const Point3i maxChunk);
  //void setChunkRange(const Point3i minChunk, const Point3i maxChunk);
  int chunkPriority(const Point3i &cp);//hash_t hash);
  
  std::unordered_map<hash_t, ChunkPtr>& getChunks();
  void lock();
  void unlock();
  
  ChunkPtr operator[](const Point3i &cp);
  ChunkPtr operator[](hash_t hash);
  
private:
  std::unordered_map<hash_t, ChunkPtr> mChunks;
  std::unordered_map<hash_t, sideFlag_t> mChunkNeighbors;
  std::unordered_set<hash_t> mLoadList; // list of chunks that need to be loaded
  std::list<Point3i> mLoadOrder; // list of chunks that need to be loaded
  std::unordered_set<hash_t> mLoadingChunks;
  std::unordered_set<hash_t> mUnloadedChunks;
  std::queue<ChunkPtr> mUnusedChunks;
  std::unordered_map<hash_t, std::unordered_map<blockSide_t, hash_t>> mNeighbors;
  
  std::unordered_set<hash_t> mNextEdge;
  std::unordered_map<hash_t, int> mTreeOrder;
  
  TreeNode mChunkTree;
  TreeNode mVisibleTree;
  
  ChunkLoader *mLoader = nullptr;
  Camera *mCamera = nullptr;
  Point3i mCamPos;
  Vector3i mRadius;

  std::atomic<int> mNumLoaded = 0;
  std::atomic<int> mNumLoading = 0;

  std::mutex mChunkLock;
  std::mutex mLoadLock;
  std::mutex mTreeLock;
  std::mutex mNeighborLock;

  void loadChunk(hash_t hash);
  void unloadChunk(hash_t hash);
  void chunkFinishedLoading(Chunk *chunk);
};

#endif // CHUNK_MAP_HPP
