#ifndef CIRCULAR_BUFFER_HPP
#define CIRCULAR_BUFFER_HPP

#include <vector>

template<typename T>
class cCircularBuffer
{
public:
  cCircularBuffer()
  { }
  cCircularBuffer(int size)
    : mSize(size), mData(size)
  { }
  ~cCircularBuffer()
  { }

  void resize(int newSize)
  {
    int diff = newSize - mSize;
    mOffset += diff / 2;
    mSize = newSize;
    mData.resize(mSize);
  }

  T& staticAccess(int index)
  { return mData[index]; }
  const T& staticAccess(int index) const
  { return mData[index]; }

  void rotate(int num)
  {
    mOffset = (mOffset + num + mSize) % mSize;
  }

  T& operator[](int index)
  {
    return mData[(index + mOffset + mSize) % mSize];
  }
  const T& operator[](int index) const
  {
    return mData[(index + mOffset + mSize) % mSize];
  }
  
private:
  int mSize = 0;
  int mOffset = 0;
  std::vector<T> mData;
};


#endif // CIRCULAR_BUFFER_HPP
