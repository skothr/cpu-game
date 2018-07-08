#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <vector>
#include <thread>

class cCpu;
class cMemory;

class cDevice
{
public:
  cDevice();

  bool addCpu(cCpu *newCpu);
  bool addMemory(cMemory *newMemory);

  bool start();
  bool stop();

  void runLoop();
  
private:
  bool mRunning = false;
  std::thread mThread;
  cMemory *mMemory;
  cCpu *mCpu;
};

#endif // DEVICE_HPP
