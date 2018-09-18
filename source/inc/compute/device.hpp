#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <vector>
#include <thread>

class Cpu;
class Memory;

class Device
{
public:
  Device();

  bool addCpu(Cpu *newCpu);
  bool addMemory(Memory *newMemory);

  bool ready() const;
  void update();
  /*
  bool start();
  bool stop();

  void runLoop();
  */
private:
  //bool mRunning = false;
  //std::thread mThread;
  Memory *mMemory = nullptr;
  Cpu *mCpu = nullptr;
};

#endif // DEVICE_HPP
