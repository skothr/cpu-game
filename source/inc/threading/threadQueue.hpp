#ifndef THREAD_QUEUE_HPP
#define THREAD_QUEUE_HPP

#include <queue>
#include <mutex>

template<typename T>
class ThreadQueue
{
public:
  ThreadQueue() { }

  void push(T *obj)
  {
    if(mLocked)
      {
        mQueue.push(obj);
      }
    else
      {
        std::lock_guard<std::mutex> lock(mLock);
        mQueue.push(obj);
      }
  }
  
  T* pop()
  {
    T *obj = nullptr;
    bool locked = mLocked;
    
    if(!locked) { lock(); }
    if(mQueue.size() > 0)
      {
        obj = mQueue.front();
        mQueue.pop();
      }
    if(!locked) { unlock(); }
    
    return obj;
  }
  int size()
  {
    if(mLocked)
      {
        return mQueue.size();
      }
    else
      {
        std::lock_guard<std::mutex> lock(mLock);
        return mQueue.size();
      }
  }

  void lock()
  {
    if(!mLocked)
      { mLock.lock(); }
    mLocked = true;
  }
  void unlock()
  {
    if(mLocked)
      { mLock.unlock(); }
    mLocked = false;
  }
  
private:
  std::queue<T*> mQueue;
  std::mutex mLock;
  bool mLocked = false;
};

template<typename T>
class RawThreadQueue
{
public:
  RawThreadQueue() { }

  void push(T obj)
  {
    if(mLocked)
      {
        mQueue.push(obj);
      }
    else
      {
        std::lock_guard<std::mutex> lock(mLock);
        mQueue.push(obj);
      }
  }
  
  T pop()
  {
    if(mQueue.size() > 0)
      {
        if(mLocked)
          {
            T obj = mQueue.front();
            mQueue.pop();
            return obj;
          }
        else
          {
            std::lock_guard<std::mutex> lock(mLock);
            T obj = mQueue.front();
            mQueue.pop();
            return obj;
          }
      }
    else
      {
        LOGE("RawThreadQueue popped with no entries!!");
        exit(1);
      }
  }
  int size()
  {
    if(mLocked)
      {
        return mQueue.size();
      }
    else
      {
        std::lock_guard<std::mutex> lock(mLock);
        return mQueue.size();
      }
  }

  void lock()
  {
    if(!mLocked)
      { mLock.lock(); }
    mLocked = true;
  }
  void unlock()
  {
    if(mLocked)
      { mLock.unlock(); }
    mLocked = false;
  }
  
private:
  std::queue<T> mQueue;
  std::mutex mLock;
  bool mLocked = false;
};


#endif // THREAD_QUEUE_HPP
