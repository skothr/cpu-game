#include "threadPool.hpp"

#include <unistd.h>

cThreadPool::cThreadPool(int numThreads, const callback_t &callback, int sleepTimeUs)
  : mNumThreads(numThreads), mCallback(callback), mSleepTimeUs(sleepTimeUs)
{ }
cThreadPool::~cThreadPool()
{
  stop();
}

void cThreadPool::start()
{
  if(!mThreadsRunning)
    {
      // start load threads
      mThreadsRunning = true;
      
      for(int i = 0; i < mNumThreads; i++)
	{ mThreads.emplace_back(&cThreadPool::threadLoop, this, i); }
    }
}

void cThreadPool::stop(bool join)
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
bool cThreadPool::running() const
{
  return mThreadsRunning;
}

void cThreadPool::threadLoop(int id)
{
  while(mThreadsRunning)
    {
      mCallback(id);
      usleep(mSleepTimeUs);
    }
}
