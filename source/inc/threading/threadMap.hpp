#ifndef THREAD_MAP_HPP
#define THREAD_MAP_HPP

#include <unordered_map>
#include <mutex>

#include "hashing.hpp"

template<typename T>
class ThreadMap
{
  static_assert(std::is_pointer<T>::value, "ThreadMap typename must be a pointer type.");
public:
  typedef typename std::unordered_map<hash_t, T>::iterator iterator_t;
  ThreadMap()
  { }
  ~ThreadMap()
  { clear(); }
  ThreadMap(ThreadMap &other)
  {
    other.lock();
    mMap = other.mMap;
    mSize = other.size;
    other.unlock();
  }
  ThreadMap& operator=(ThreadMap &other)
  {
    lock();
    other.lock();
    mMap = other.mMap;
    mSize = other.size;
    other.unlock();
    unlock();
    return *this;
  }

  
  void lock()
  {
    mLock.lock();
    mLocked = true;
  }
  void unlock()
  {
    mLocked = false;
    mLock.unlock();
  }
  bool isLocked() const
  { return mLocked; }

  int size() const
  { return mSize; }
  
  // NOTE: Lock before using these!!
  iterator_t begin()
  { return mMap.begin(); }
  iterator_t end()
  { return mMap.end(); }
  const iterator_t begin() const
  { return mMap.begin(); }
  const iterator_t end() const
  { return mMap.end(); }

  bool contains(hash_t hash)
  {
    std::lock_guard<std::mutex> lock(mLock);
    return (mMap.find(hash) != mMap.end());
  }
  T at(hash_t hash)
  {
    std::lock_guard<std::mutex> lock(mLock);
    auto iter = mMap.find(hash);
    if(iter != mMap.end())
      { return iter->second; }
    else
      { return nullptr; }
  }
  T operator[](hash_t hash)
  {
    std::lock_guard<std::mutex> lock(mLock);
    auto iter = mMap.find(hash);
    if(iter != mMap.end())
      { return iter->second; }
    else
      { return nullptr; }
  }

  void emplace(hash_t hash, T obj)
  {
    std::lock_guard<std::mutex> lock(mLock);
    auto iter = mMap.find(hash);
    if(iter != mMap.end() && iter->second)
      {
        iter->second = obj;
      }
    else
      {
        mMap.emplace(hash, obj);
        mSize++;
      }
  }
  void erase(hash_t hash)
  {
    std::lock_guard<std::mutex> lock(mLock);
    auto iter = mMap.find(hash);
    if(iter != mMap.end())
      {
        mMap.erase(hash);
        mSize--;
      }
  }
  void clear()
  {
    std::lock_guard<std::mutex> lock(mLock);
    mMap.clear();
    mSize = 0;
  }



  
  bool lockedContains(hash_t hash)
  {
    return (mMap.find(hash) != mMap.end());
  }
  void lockedEmplace(hash_t hash, T obj)
  {
    auto iter = mMap.find(hash);
    if(iter == mMap.end())
      { mSize++; }
    mMap[hash] = obj;
  }
  T lockedAt(hash_t hash)
  {
    auto iter = mMap.find(hash);
    if(iter != mMap.end())
      { return iter->second; }
    else
      { return nullptr; }
  }
  void lockedErase(hash_t hash)
  {
    auto iter = mMap.find(hash);
    if(iter != mMap.end())
      {
        mMap.erase(hash);
        mSize--;
      }
  }
  void lockedClear()
  {
    mMap.clear();
    mSize = 0;
  }
  
  
  
  
private:
  std::mutex mLock;
  bool mLocked = false;
  int mSize = 0;
  std::unordered_map<hash_t, T> mMap;

};

#endif // THREAD_MAP_HPP
