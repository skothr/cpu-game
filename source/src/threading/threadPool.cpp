#include "threadPool.hpp"

#include <unistd.h>

ThreadPool::ThreadPool(int numThreads, const callback_t &callback, int sleepTimeUs)
  : mNumThreads(numThreads), mCallback(callback), mSleepTimeUs(sleepTimeUs)
{ }
ThreadPool::~ThreadPool()
{
  stop();
}

void ThreadPool::start()
{
  if(!mThreadsRunning)
    {
      // start load threads
      mThreadsRunning = true;
      
      for(int i = 0; i < mNumThreads; i++)
	{ mThreads.emplace_back(&ThreadPool::threadLoop, this, i); }
    }
}

void ThreadPool::stop(bool join)
{
  mThreadsRunning = false;
  if(join)
    {
      // start load threads
      for(auto &thread : mThreads)
        { thread.join(); }
      mThreads.clear();
    }
}
void ThreadPool::threadLoop(int id)
{
  while(mThreadsRunning)
    {
      mCallback(id);
      usleep(mSleepTimeUs);
    }
}
