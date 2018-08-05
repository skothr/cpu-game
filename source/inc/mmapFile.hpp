#ifndef MMAP_FILE_HPP
#define MMAP_FILE_HPP

#include <sys/types.h>
#include <string>

class cMmapFile
{
public:
  static const int NONE = 0x00;
  static const int INPUT = 0x01;
  static const int OUTPUT = 0x02;
  static const int CREATE = 0x04;
  static const int TRUNC = 0x08;
  
  cMmapFile();
  ~cMmapFile();

  int read(int offset, uint8_t *dataOut, int length);
  int write(int offset, const uint8_t *data, int length);

  bool open(const std::string &filePath, int ioType, bool shared, int size = 0);
  void close();
  bool resize(int newSize);

  bool isOpen() const;
  int size() const;
  
private:
  std::string mFilePath;
  int mFd = -1;
  int mIoType;
  const uint8_t *mFileData = nullptr;
  size_t mFileLength = 0;
  bool mShared = false;
  
};

#endif // MMAP_FILE_HPP
