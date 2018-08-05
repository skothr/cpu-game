#ifndef REGION_FILE_HPP
#define REGION_FILE_HPP

#include "block.hpp"
#include "chunk.hpp"
#include "worldFile.hpp"

#include <sys/types.h>
#include <string>
#include <array>
#include <mutex>
#include <atomic>

class cRegionFile
{
public: 
  cRegionFile(const std::string &filePath, const Vector<uint8_t, 4> &version, bool create = false);
  ~cRegionFile();

  int size() const             { return mFileSize; }
  bool isOpen() const          { return (mFd >= 0); }
  operator bool() const        { return isOpen(); }
  
  Point3i getRegionPos() const { return mRegionPos; }
  bool readChunk(cChunk *chunk);
  bool writeChunk(const cChunk *chunk);
  void close();
  
private:
  // region file stuff
  const Vector<uint8_t, 4> mVersion;
  std::atomic<uint32_t> mNextOffset;
  wData::Header mHeader;
  std::array<wData::ChunkInfo, CHUNKS_PER_REGION> mChunkInfo;
  std::array<std::atomic<bool>, CHUNKS_PER_REGION> mChunkStatus;
  Point3i mRegionPos;
  std::vector<uint8_t> mChunkData;

  // mmap file stuff
  std::string mFilePath;
  int mFd = -1;
  size_t mFileSize = 0;
  uint8_t *mFileData = nullptr;
  std::mutex mMapLock;

  bool openFile();
  bool createFile();
  bool resizeFile(int extraBytes);
  
  void calcRegionPos();
  bool readRegion();
  bool createRegion();

  void readVersion();
  void writeVersion();
  void readLookup(int chunkIndex);
  void writeLookup(int chunkIndex);
  void readLookup();
  void writeLookup();
  void readOffset();
  void writeOffset();
  void readChunkData(int chunkIndex);
  void writeChunkData(int chunkIndex);
  
  int chunkIndex(int bx, int by, int bz);
  int chunkIndex(const Point3i cp);
  Point3i unflattenChunkIndex(int index);
};

#endif // REGION_FILE_HPP
