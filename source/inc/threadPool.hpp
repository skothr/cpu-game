#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <thread>
#include <functional>
#include <vector>

class cThreadPool
{
public:
  typedef std::function<void(int id)> callback_t;

  cThreadPool(int numThreads, const callback_t &callback, int sleepTimeUs = 1000);
  ~cThreadPool();

  void start();
  void stop(bool join = true);

  bool running() const;
  int numThreads() const { return mNumThreads; }
  
private:
  int mNumThreads;
  int mSleepTimeUs;
  callback_t mCallback;
  std::vector<std::thread> mThreads;
  
  bool mThreadsRunning = false;

  void threadLoop(int id);
};

#endif // THREAD_POOL_HPP
