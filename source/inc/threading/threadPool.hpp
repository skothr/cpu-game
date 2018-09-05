#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <thread>
#include <functional>
#include <vector>

class ThreadPool
{
public:
  typedef std::function<void(int id)> callback_t;

  ThreadPool(int numThreads, const callback_t &callback, int sleepTimeUs = 10000);
  ~ThreadPool();

  void start();
  void stop(bool join = true);

  void setThreads(int numThreads)
  { mNumThreads = numThreads; }

  bool running() const   { return mRunning; }
  int numThreads() const { return mNumThreads; }
  
private:
  int mNumThreads;
  int mSleepTimeUs;
  std::vector<std::thread> mThreads;
  bool mRunning = false;
  callback_t mCallback;

  void threadLoop(int id);
};

#endif // THREAD_POOL_HPP
