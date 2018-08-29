#include "device.hpp"
#include "logging.hpp"
#include "cpu.hpp"
#include "memory.hpp"

#include <unistd.h>
#include <chrono>
#define CLOCK std::chrono::high_resolution_clock

#define THREAD_SLEEP_US (1000 * 50) // 50ms

cDevice::cDevice()
{

}

bool cDevice::addCpu(cCpu *newCpu)
{
  if(mRunning)
    {
      LOGI("Failed to add CPU -- device running.");
      return false;
    }
  else
    {
      mCpu = newCpu;
      return true;
    }
}

bool cDevice::addMemory(cMemory *newMemory)
{
  mMemory = newMemory;
  return true;
}

bool cDevice::start()
{
  mRunning = true;
  mThread = std::thread( [this]{ runLoop(); } );
  return true;
}
bool cDevice::stop()
{
  mRunning = false;
  mThread.join();
  return true;
}

void cDevice::runLoop()
{
  auto lastTime = CLOCK::now();
  while(mRunning)
    {
      auto now = CLOCK::now();
      std::chrono::duration<double> elapsed = now - lastTime;
      mCpu->update(elapsed.count(), mMemory);

      lastTime = now;
      usleep(THREAD_SLEEP_US);
    }
}
