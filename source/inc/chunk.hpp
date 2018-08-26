#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "chunkData.hpp"
#include "chunkMesh.hpp"
#include "vector.hpp"
#include <unordered_map>
#include <string>
#include <ostream>
#include <atomic>

// NOTE:
//  - wx/wy/wz denotes block position within world
//  - bx/by/bz denotes block position within chunk


class cChunk
{
public:
  static const int sizeX = cChunkData::sizeX;
  static const int sizeY = cChunkData::sizeY;
  static const int sizeZ = cChunkData::sizeZ;
  static const int totalSize = cChunkData::totalSize;
  static const Point3i size;
  
  cChunk();
  cChunk(const Point3i &worldPos);
  cChunk(const cChunk &other);
  ~cChunk();

  bool verticalLoaded(int maxZ)
  {
    return (mWorldPos[2] == maxZ ||
            (mNeighbors[blockSide_t::PZ] &&
             mNeighbors[blockSide_t::PZ]->verticalLoaded(maxZ)));
  }
  cChunk* highestDirtyChunk()
  {
    if(!isLight() && mNeighbors[blockSide_t::PZ])
      {
        return mNeighbors[blockSide_t::PZ]->highestDirtyChunk();
      }
    else
      {
        return this;
      }
  }

  void updateBelow()
  {
    if(mNeighbors[blockSide_t::NZ])
      {
        mNeighbors[blockSide_t::NZ]->setLight(true);
        mNeighbors[blockSide_t::NZ]->setMeshDirty(true);
        mNeighbors[blockSide_t::NZ]->updateBelow();
        //mNeighbors[blockSide_t::NZ]->updateLighting();
      }
  }

  void setNeighbor(blockSide_t side, cChunk *chunk);
  void setNeighborMirror(blockSide_t side, cChunk *chunk);
  void unsetNeighbor(blockSide_t side);
  void unsetNeighborMirror(blockSide_t side);
  void unsetNeighbors();
  cChunk* getNeighbor(blockSide_t side);

  void setWorldPos(const Point3i &newPos);
  
  Point3i pos() const;
  bool empty() const;

  cBlock* at(int bx, int by, int bz, cChunk **neighbor = nullptr);
  cBlock* at(Point3i bp, cChunk **neighbor = nullptr);
  block_t get(int bx, int by, int bz) const;
  block_t get(const Point3i &bp) const;
  
  bool set(int bx, int by, int bz, block_t type);
  bool set(const Point3i &bp, block_t type);
  
  int serialize(uint8_t *dataOut) const;
  void deserialize(const uint8_t *dataIn, int bytes);

  void updateLighting(int lighting); // should be called after PZ neighbor has been updated.
  int8_t getLighting(const Point3i &bp, const Point3i &vp, blockSide_t side);
  void updateBlocks();

  void initGL(cShader *shader);
  void cleanupGL();
  void render();

  void clearMesh();
  void updateMesh();
  void uploadMesh();
  
  // atomic functions for loading and saving
  bool isLight() const { return mLighting; }
  void setLight(bool lighting)  { mLighting = lighting; }
  bool isLoaded() const;
  void setLoaded(bool loaded, bool reset = true);
  bool neighborsLoaded() const;
  bool checkNeighbors();
  bool isDirty() const;
  void setDirty(bool dirty);
  bool meshDirty() const;
  void setMeshDirty(bool dirtyMesh);
  bool meshUploaded() const;
  void setMeshUploaded(bool uploaded);

  static int chunkX(int wx);
  static int chunkY(int wy);
  static int chunkZ(int wz);
  static Point3i chunkPos(const Point3i &wp);
  
  std::string toString() const;
  friend std::ostream& operator<<(std::ostream &os, const cChunk &chunk);
  
private:
  cChunkData mData;
  cChunkMesh *mMesh;
  std::vector<cSimpleVertex> mVert;
  std::vector<unsigned int> mInd;
  Point3i mWorldPos;
  std::unordered_map<blockSide_t, cChunk*> mNeighbors;

  std::atomic<bool> mLighting = false;  // chunk should be destroyed
  std::atomic<bool> mLoaded = false;  // chunk is loaded
  std::atomic<bool> mNeighborsLoaded = false;  // chunk neighbors are loaded
  std::atomic<bool> mDirty = false;   // chunk has changed and needs to be saved
  std::atomic<bool> mMeshDirty = true;   // chunk has changed and needs to be saved
  std::atomic<bool> mMeshUploaded = false;   // mesh is rendered, but not uploaded to the GPU
  
  void updateOcclusion(int bx, int by, int bz);
  
  int index(const Point3i &bp);
  int expand(int x);
  int unexpand(int x) const;
  int hashBlock(int bx, int by, int bz);
  int hashBlock(const Point3i &bp);
};


#endif // CHUNK_HPP
