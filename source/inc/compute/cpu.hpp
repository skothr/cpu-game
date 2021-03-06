#ifndef CPU_HPP
#define CPU_HPP

#include <vector>
#include <cstdint>

enum Instruction
  {
   LOADL = 0x00, // load literal
   LOADA,        // load memory
   ADD,
   SUBTRACT,
   STORE,
   JUMP,
   STOP
  };

class Memory;

class Cpu
{
public:
  const int bits;  // number of bits
  const int regs;  // number of registers
  const int speed; // speed of operations (ops/sec)
  const int maxVal;
private:
  const double tickTime;

public:
  Cpu(int numBits, int numRegs, int cpuSpeed);

  void runProgram(int addr);
  bool update(double dt, Memory *memory);
  
private:
  double mRemaining = 0.0;

  int mProgramCounter = -1;
  uint8_t mInstructionReg = 0x00;
  uint8_t mTempReg = 0x00;
  uint8_t *mRegisters = nullptr;

  bool mOverflow = false;
  bool mUnderflow = false;
  
  void tick(Memory *memory);
};

#endif // CPU_HPP
