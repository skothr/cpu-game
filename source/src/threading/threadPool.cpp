#include "threadPool.hpp"

#include <unistd.h>

#include "logging.hpp"

ThreadPool::ThreadPool(int numThreads, const callback_t &callback, int sleepTimeUs)
  : mNumThreads(numThreads), mCallback(callback), mSleepTimeUs(sleepTimeUs)
{ }
ThreadPool::~ThreadPool()
{ stop(true); }

void ThreadPool::start()
{
  if(!mRunning)
    {
      // start load threads
      mRunning = true;
      mThreads.reserve(mNumThreads);
      for(int i = 0; i < mNumThreads; i++)
	{ mThreads.emplace_back(&ThreadPool::threadLoop, this, i); }
    }
}

void ThreadPool::stop(bool join)
{
  mRunning = false;
  if(join)
    { // stop join with threads
      for(int i = 0; i < mThreads.size(); i++)
        { mThreads[i].join(); }
      mThreads.clear();
    }
}
void ThreadPool::threadLoop(int id)
{
  while(mRunning)
    {
      if(mCallback)
        { mCallback(id); }
      usleep(mSleepTimeUs);
    }
}
