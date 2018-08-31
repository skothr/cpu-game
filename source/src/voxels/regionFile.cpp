#include "regionFile.hpp"

#include <sstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

RegionFile::RegionFile(const std::string &filePath, const Vector<uint8_t, 4> &version,
                         bool create )
  : mFilePath(filePath), mVersion(version)
{
  bool success;
  if(create)
    { success = createRegion(); }
  else
    { success = readRegion(); }

  if(!success)
    {
      LOGE("Region file failed!");
      exit(1);
    }
  
  calRegionPos();
}

RegionFile::~RegionFile()
{
  close();
}

void RegionFile::close()
{

  if(munmap(mFileData, mFileSize) < 0)
    {
      LOGE("Failed to close region file!");
    }
  
  ::close(mFd);
}

bool RegionFile::openFile()
{
  std::lock_guard<std::mutex> lock(mMapLock);
  LOGD("Opening...");
  mFd = ::open(mFilePath.c_str(), O_RDWR, (mode_t)0666);
  
  if(mFd < 0)
    {
      LOGE("Failed to open region file '%s'.", mFilePath.c_str());
      return false;
    }
  
  // obtain file size
  struct stat sb;
  if(fstat(mFd, &sb) < 0)
    {
      LOGE("Failed to query file size of '%s'.", mFilePath.c_str());
      ::close(mFd);
      mFd = -1;
      return false;
    }
  mFileSize = sb.st_size;
  
  if(mFileSize == 0)
    {
      LOGD("Region file contains no data!");
      return false;
    }
  mFileData = (uint8_t*)(mmap(NULL, mFileSize, PROT_READ | PROT_WRITE, MAP_SHARED, mFd, 0));
  if(mFileData == MAP_FAILED)
    {
      LOGE("Failed to map '%s' to memory --> %d.", mFilePath.c_str(), errno);
      ::close(mFd);
      mFd = -1;
      return false;
    }
  return true;
}

bool RegionFile::createFile()
{
  std::lock_guard<std::mutex> lock(mMapLock);
  LOGD("Creating region file...");
  mFd = ::open(mFilePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, (mode_t)0666);
  
  if(mFd < 0)
    {
      LOGE("Failed to create region file '%s'.", mFilePath.c_str());
      return false;
    }

  // initial file sizing (header + lookup + room for one chunk)
  mFileSize = (sizeof(wData::Header) + sizeof(wData::ChunkInfo) * CHUNKS_PER_REGION +
               Chunk::totalSize * Block::dataSize );
  if(ftruncate(mFd, mFileSize) < 0)
    {
      ::close(mFd);
      mFd = -1;
      LOGE("Error calling lseek() to size region file");
      return false;
    }
  fsync(mFd); // flush
  mFileData = (uint8_t*)(mmap(0, mFileSize, PROT_READ | PROT_WRITE, MAP_SHARED, mFd, 0 ));
  if(mFileData == MAP_FAILED)
    {
      LOGE("Failed to map '%s' to memory --> %d.", mFilePath.c_str(), errno);
      ::close(mFd);
      mFd = -1;
      return false;
    }

  return true;
}

bool RegionFile::resizeFile(int extraBytes)
{
  //std::lock_guard<std::mutex> lock(mMapLock);
  if(ftruncate(mFd, mFileSize + extraBytes) < 0)
    {
      LOGE("Failed to resize region file!");
      return false;
    }
  fsync(mFd); // flush

  { // resize file
    //std::lock_guard<std::mutex> lock(mMapLock);
    mFileData = (uint8_t*)mmap(0, mFileSize + extraBytes, PROT_READ | PROT_WRITE,
                               MAP_SHARED, mFd, 0 );
    if(mFileData == MAP_FAILED)
      {
        LOGE("Failed to remap region file memory. Error: %d.", errno);
        return false; // TODO: File broken?
      }
    mFileSize += extraBytes;
    return true;
  }
}
bool RegionFile::readRegion()
{
  if(!openFile())
    { return false; }
  readVersion();
  if(mHeader.version != mVersion)
    {
      LOGE("Region file at has unsupported version!", mFilePath.c_str());
      std::cout << "    --> " << (int)mHeader.version[0] << "." << (int)mHeader.version[1]
                << "." <<  (int)mHeader.version[2] << "." << (int)mHeader.version[3] << "\n";
      return false;
    }
  readOffset();
  readLookup();
  for(auto &d : mChunkInfo)
    {
      if(d.chunkSize != 0)
        { std::cout << "Found chunk: " << d.offset << " | " << d.chunkSize << "\n"; }
    }
  
  return true;
}

bool RegionFile::createRegion()
{
  LOGD("Creating region...");
  if(!createFile())
    { return false; }

  mNextOffset.store(sizeof(wData::Header) + sizeof(wData::ChunkInfo) * CHUNKS_PER_REGION);
  writeVersion();
  writeOffset();

  LOGD("Offset: %d", mHeader.nextOffset);

  std::fill(mChunkInfo.begin(), mChunkInfo.end(), wData::ChunkInfo{0, 0});
  writeLookup();

  return true;
}


bool RegionFile::readChunk(Chunk *chunk)
{
  const Point3i chunkPos = chunk->pos();
  //std::cout << "READING CHUNK: " << chunkPos << "\n";
  const Point3i cPos{chunkPos[0] & (15),
                     chunkPos[1] & (15),
                     chunkPos[2] & (15) };
  const uint16_t cIndex = chunkIndex(cPos);
  //std::cout << "Chunk index: " << cIndex << " : " << cPos << "\n";

  if(!mChunkStatus[cIndex].exchange(true))
    {
      mChunkData[cIndex].clear();

      if(mChunkInfo[cIndex].chunkSize != 0)
        { // read chunk data
          mChunkData[cIndex].resize(mChunkInfo[cIndex].chunkSize);
          readChunkData(cIndex);
          //LOGD("READING CHUNK (size: %d)", mChunkInfo[cIndex].chunkSize);
      
          // deserialize chunk
          chunk->deserialize(mChunkData[cIndex]);
          mChunkStatus[cIndex].store(false);
          return true;
        }
      else // chunk not created yet
        {
          mChunkStatus[cIndex].store(false);
          return false;
        }
    }
  return false;
}

bool RegionFile::writeChunk(const Chunk *chunk)
{
  //std::lock_guard<std::mutex> lock(mMapLock);
  const Point3i chunkPos = chunk->pos();
  std::cout << "WRITING CHUNK: " << chunkPos << "\n";
  const Point3i cPos{chunkPos[0] & (15),
                     chunkPos[1] & (15),
                     chunkPos[2] & (15) };
  const uint16_t cIndex = chunkIndex(cPos);
  std::cout << "Chunk index: " << cIndex << " : " << cPos << "\n";
  
  if(!mChunkStatus[cIndex].exchange(true))
    {
      // get chunk data
      mChunkData[cIndex].clear();
      const int dataSize = chunk->serialize(mChunkData[cIndex]); // max chunk size
  
      if(mChunkInfo[cIndex].chunkSize == 0)
        { // add chunk to end of file
          mHeader.nextOffset = mNextOffset.load();
          mChunkInfo[cIndex].offset = mNextOffset.load();
          mChunkInfo[cIndex].chunkSize = dataSize;

          LOGD("NEXT OFFSET: %d", mNextOffset.load());
  
          LOGD("Writing to offset %d (%d bytes)", mChunkInfo[cIndex].offset, mChunkInfo[cIndex].chunkSize);
          // overwrite chunk
          LOGD("Writing lookup data...");
          writeLookup(cIndex);
          LOGD("Writing chunk data...");
          writeChunkData(cIndex);
      
          mNextOffset.store(mHeader.nextOffset + dataSize);
          LOGD("Next offset: %d", mHeader.nextOffset);
          writeOffset();
      
          if(!resizeFile(dataSize))
            { 
              mChunkStatus[cIndex].store(false);
              return false;
            }
        }
      else
        {
          mChunkInfo[cIndex].chunkSize = dataSize;
          LOGD("Writing to offset %d (%d bytes)", mChunkInfo[cIndex].offset, mChunkInfo[cIndex].chunkSize);
          // overwrite chunk
          LOGD("Writing lookup data...");
          writeLookup(cIndex);
          LOGD("Writing chunk data...");
          writeChunkData(cIndex);
        }
      mChunkStatus[cIndex].store(false);
  }
  
  return true;
}

std::string RegionFile::regionFileName(const Point3i &regionPos)
{
  std::ostringstream ss;
  ss << "r." << regionPos[0] << "." << regionPos[1] << "." << regionPos[2]
     << ".wr";
  return ss.str();
}
void RegionFile::calRegionPos()
{
  std::istringstream ss(mFilePath);
  std::istringstream convert;
  std::string token;
  std::getline(ss, token, '.'); // first is "r."
  std::getline(ss, token, '.'); // x
  std::cout << "X:" << token << "\n";
  convert.clear();
  convert.str(token);
  convert >> mRegionPos[0];
  
  std::getline(ss, token, '.'); // y
  std::cout << "Y:" << token << "\n";
  convert.clear();
  convert.str(token);
  convert >> mRegionPos[1];
  
  std::getline(ss, token, '.'); // z
  std::cout << "Z:" << token << "\n";
  convert.clear();
  convert.str(token);
  convert >> mRegionPos[2];

}


inline int RegionFile::chunkIndex(int bx, int by, int bz)
{ return bx + REGION_SIZEX * (bz + REGION_SIZEZ * by); }
inline int RegionFile::chunkIndex(const Point3i cp)
{ return chunkIndex(cp[0], cp[1], cp[2]); }
inline Point3i RegionFile::unflattenChunkIndex(int index)
{
  const int yi = index / REGION_SIZEY;
  index -= yi * REGION_SIZEY;
  const int zi = index / REGION_SIZEX;
  const int xi = index - zi * REGION_SIZEX;
  return Point3i{xi, yi, zi};
}

inline void RegionFile::readVersion()
{
  std::memcpy((void*)&mHeader.version, (void*)mFileData, sizeof(wData::Header::version));
}
inline void  RegionFile::writeVersion()
{
  mHeader.version = mVersion;
  std::memcpy((void*)mFileData, (void*)&mHeader.version, sizeof(wData::Header::version));
}
inline void RegionFile::readOffset()
{
  std::memcpy((void*)&mHeader.nextOffset, (void*)(mFileData + sizeof(wData::Header::version)),
              sizeof(wData::Header::nextOffset));
  mNextOffset.store(mHeader.nextOffset);
}
inline void RegionFile::writeOffset()
{
  mHeader.nextOffset = mNextOffset.load();
  std::memcpy(((void*)mFileData + sizeof(wData::Header::version)),
              (void*)&mHeader.nextOffset, sizeof(wData::Header::nextOffset) );
}
inline void RegionFile::readLookup(int chunkIndex)
{
  const int offset = sizeof(wData::Header) + sizeof(wData::ChunkInfo) * chunkIndex;
  std::memcpy((void*)&mChunkInfo[chunkIndex], (void*)(mFileData + offset), sizeof(wData::ChunkInfo));
}
inline void RegionFile::writeLookup(int chunkIndex)
{
  const int offset = sizeof(wData::Header) + sizeof(wData::ChunkInfo) * chunkIndex;
  std::memcpy((void*)(mFileData + offset), (void*)&mChunkInfo[chunkIndex], sizeof(wData::ChunkInfo));
}
inline void RegionFile::readLookup()
{
  const int offset = sizeof(wData::Header);
  const int bytes = sizeof(wData::ChunkInfo) * CHUNKS_PER_REGION;
  std::memcpy((void*)&mChunkInfo[0], (void*)(mFileData + offset), bytes);
}
inline void RegionFile::writeLookup()
{
  const int offset = sizeof(wData::Header);
  const int bytes = sizeof(wData::ChunkInfo) * CHUNKS_PER_REGION;
  std::cout << "Writing lookup at: " << offset << " (" << bytes  << " bytes)\n";
  std::memcpy((void*)(mFileData + offset), (void*)&mChunkInfo[0], bytes);
}
inline void RegionFile::readChunkData(int chunkIndex)
{
  std::memcpy((void*)&mChunkData[chunkIndex][0], (void*)(mFileData + mChunkInfo[chunkIndex].offset),
              mChunkInfo[chunkIndex].chunkSize );
}
inline void RegionFile::writeChunkData(int chunkIndex)
{
  std::cout << "Writing chunk at: " << mChunkInfo[chunkIndex].offset << " (" << mChunkInfo[chunkIndex].chunkSize  << " bytes)\n";
  std::memcpy((void*)(mFileData + mChunkInfo[chunkIndex].offset),
                (void*)&mChunkData[chunkIndex][0], mChunkInfo[chunkIndex].chunkSize );
}
