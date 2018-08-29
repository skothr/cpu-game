#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <thread>
#include <functional>
#include <vector>

class ThreadPool
{
public:
  typedef std::function<void(int id)> callback_t;

  ThreadPool(int numThreads, const callback_t &callback, int sleepTimeUs = 1000);
  ~ThreadPool();

  void start();
  void stop(bool join = true);

  bool running() const   { return mThreadsRunning; }
  int numThreads() const { return mNumThreads; }
  
private:
  const int mNumThreads;
  const int mSleepTimeUs;
  std::vector<std::thread> mThreads;
  bool mThreadsRunning = false;
  callback_t mCallback;

  void threadLoop(int id);
};

#endif // THREAD_POOL_HPP
