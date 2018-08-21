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

  void setNeighbor(blockSide_t side, cChunk *chunk);
  void unsetNeighbor(blockSide_t side);
  void unsetNeighbors();
  cChunk* getNeighbor(blockSide_t side);

  void setWorldPos(const Point3i &newPos);
  
  Point3i pos() const;
  bool empty() const;

  cBlock* at(int bx, int by, int bz);
  cBlock* at(Point3i bp);
  block_t get(int bx, int by, int bz) const;
  block_t get(const Point3i &bp) const;
  
  bool set(int bx, int by, int bz, block_t type);
  bool set(const Point3i &bp, block_t type);
  
  int serialize(uint8_t *dataOut) const;
  void deserialize(const uint8_t *dataIn, int bytes);

  uint8_t getLighting(const Point3i &bp, const Point3i &vp, blockSide_t side);
  void updateBlocks();

  void initGL(cShader *shader);
  void cleanupGL();
  void render();
  
  void updateMesh();
  void uploadMesh();
  
  // atomic functions for loading and saving
  bool isLoaded() const;
  void setLoaded(bool loaded);
  bool isDirty() const;
  void setDirty(bool dirty);
  bool meshDirty() const;
  void setMeshDirty(bool dirtyMesh);
  bool meshUploaded() const;
  void setMeshUploaded(bool uploaded);
  
  std::string toString() const;
  friend std::ostream& operator<<(std::ostream &os, const cChunk &chunk);
  
private:
  cChunkData mData;
  cChunkMesh *mMesh;
  std::vector<cSimpleVertex> mVert;
  std::vector<unsigned int> mInd;
  Point3i mWorldPos;
  std::unordered_map<blockSide_t, cChunk*> mNeighbors;

  std::atomic<bool> mLoaded = false;  // chunk is being loaded
  std::atomic<bool> mDirty = false;   // chunk has changed and needs to be saved
  std::atomic<bool> mMeshDirty = true;   // chunk has changed and needs to be saved
  std::atomic<bool> mMeshUploaded = false;   // mesh is rendered, but not uploaded to the GPU
  
  void updateLighting(int bx, int by, int bz);
  
  int index(const Point3i &bp);
};


#endif // CHUNK_HPP
