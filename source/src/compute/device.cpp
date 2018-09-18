#include "device.hpp"
#include "logging.hpp"
#include "cpu.hpp"
#include "memory.hpp"

#include <unistd.h>
#include <chrono>
#define CLOCK std::chrono::high_resolution_clock

#define THREAD_SLEEP_US (1000 * 50) // 50ms

Device::Device()
{

}

bool Device::addCpu(Cpu *newCpu)
{
  //if(mRunning)
  //   {
  //     LOGI("Failed to add CPU -- device running.");
  //     return false;
  //   }
  // else
  //   {
      mCpu = newCpu;
      return true;
      //}
}

bool Device::addMemory(Memory *newMemory)
{
  mMemory = newMemory;
  return true;
}
bool Device::ready() const
{
  return (mMemory && mCpu);
}

void Device::update()
{
  mCpu->update(1.0/100.0, mMemory);
}

/*
bool Device::start()
{
  mRunning = true;
  mThread = std::thread( [this]{ runLoop(); } );
  return true;
}
bool Device::stop()
{
  mRunning = false;
  mThread.join();
  return true;
}

void Device::runLoop()
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
*/
