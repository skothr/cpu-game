#include "mmapFile.hpp"
#include "logging.hpp"

#define _GNU_SOURCE


#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>


cMmapFile::cMmapFile()
{ }

cMmapFile::~cMmapFile()
{
  close();
}


bool cMmapFile::open(const std::string &filePath, int ioType, bool shared, int size)
{
  close();
  mFilePath = filePath;

  mIoType = ioType;
  int openType = (((ioType & INPUT) && (ioType | OUTPUT) ? (O_RDWR) :
		   (ioType & INPUT ? O_RDONLY : (ioType & OUTPUT ? O_WRONLY : 0) )) |
		  (ioType & TRUNC ? O_TRUNC : 0 ) |
		  (ioType & CREATE ? O_CREAT : 0 ));
  mShared = shared;
  
  LOGD("Opening...");
  mFd = ::open(mFilePath.c_str(), openType, (mode_t)0666);//O_RDWR);
  
  if(mFd < 0)
    {
      
      LOGE("Failed to open file '%s'.", mFilePath.c_str());
      ::close(mFd);
      mFd = -1;
      mFilePath = "";
      return false;
    }
  else
    { LOGD("Opened file."); }
  
  // obtain file size
  struct stat sb;
  if(fstat(mFd, &sb) < 0)
    {
      LOGE("Failed to query file size of '%s'.", mFilePath.c_str());
      ::close(mFd);
      mFd = -1;
      mFilePath = "";
      return false;
    }
  mFileLength = sb.st_size;
  LOGD("File size: %d.", mFileLength);
  if(mFileLength == 0 && size > 0)
    {
      mFileLength = size;
      if(lseek(mFd, mFileLength, SEEK_SET) == -1)
	{
	  ::close(mFd);
	  LOGE("Error calling lseek() to 'stretch' the file");
	  return false;
	}
      if(::write(mFd, "", 1) == -1)
	{
	  ::close(mFd);
	  LOGE("Error writing last byte of the file");
	  return false;
	}
      //ftruncate(mFd, mFileLength);
    }
  mFileData = static_cast<const uint8_t*>(mmap(NULL, mFileLength, //PROT_READ | PROT_WRITE,
					       ((ioType | INPUT ? PROT_READ : 0) |
						(ioType | OUTPUT ? PROT_WRITE : 0)),
					       (mShared ? MAP_SHARED : 0), mFd, 0 ));
  if(mFileData == MAP_FAILED)
    {
      LOGE("Failed to map '%s' to memory --> %d.", mFilePath.c_str(), errno);
      ::close(mFd);
      mFd = -1;
      mFilePath = "";
      return false;
    }
  else
    { LOGD("Map succeeded."); }

  return true;
}

void cMmapFile::close()
{
  LOGD("CLOSING MMAP FILE...");
  if(isOpen())
    {
      if(munmap((void*)mFileData, mFileLength) < 0)
        {
          LOGE("Unmapping memory failed!\n");
        }
      if(::close(mFd) < 0)
        {
          LOGD("Closing file descriptor failed!\n");
        }
      mFilePath = "";
      mFileLength = 0;
      mFileData = nullptr;
      mFd = -1;
    }
  LOGD("DONE CLOSING MMAP FILE");
}
bool cMmapFile::isOpen() const
{
  return (mFd >= 0);
}

int cMmapFile::size() const
{ return mFileLength; }
int cMmapFile::read(int offset, uint8_t *dataOut, int length)
{
  if(isOpen())
    {
      std::memcpy((void*)dataOut, (void*)(mFileData + offset), length);
      return length;
    }
  else
    { return false; }
}
int cMmapFile::write(int offset, const uint8_t *data, int length)
{
  if(isOpen())
    {
      std::memcpy((void*)(mFileData + offset), (void*)data, length);
      return length;
    }
  else
    { return false; }
}

bool cMmapFile::resize(int newSize)
{
  if(isOpen())
    {
      munmap((void*)mFileData, mFileLength);
      mFileLength = newSize;
      ftruncate(mFd, mFileLength);
      mFileData = static_cast<const uint8_t*>(mmap(NULL, mFileLength, //PROT_READ | PROT_WRITE,
                                                   ((mIoType | INPUT ? PROT_READ : 0) |
                                                    (mIoType | OUTPUT ? PROT_WRITE : 0)),
                                                   (mShared ? MAP_SHARED : 0), mFd, 0 ));

  
      //mFileData = static_cast<const char*>(mremap((void*)mFileData, mFileLength, newSize, MREMAP_MAYMOVE));
      //LOGD("new file: %d", (long)mFileData);
      //if(mFileData == MAP_FAILED)
      //if(mremap((void*)mFileData, mFileLength, newSize, 0) < 0)

      if(mFileData == MAP_FAILED)
        {
          LOGD("Failed to resize mapped file from %d to %d. Errno --> %d", mFileLength, newSize, errno);
          return false;
        }
      else
        {
          mFileLength = newSize;
          return true;
        }
    }
  else
    {
      return false;
    }
}
