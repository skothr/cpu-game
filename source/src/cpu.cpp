#include "cpu.hpp"
#include "logging.hpp"
#include "memory.hpp"

#include <cmath>


cCpu::cCpu(int numBits, int numRegs, int cpuSpeed)
  : bits(numBits), regs(numRegs), speed(cpuSpeed), maxVal(1 << (numBits-1)),
    tickTime(1.0 / (double)cpuSpeed)
{
  if(bits == 8)
    { mRegisters = (uint8_t*)(new int8_t[numRegs]); }
  /*
    else if(bits == 16)
    { mRegisters = (uint8_t*)(new int16_t[numRegs]); }
    else if(bits == 16)
    { mRegisters = (uint8_t*)(new int32_t[numRegs]); }
  */
  else
    {
      LOGE("Invalid cpu bit depth --> %d", bits);
    }
}

bool cCpu::update(double dt, cMemory *memory)
{
  mRemaining += dt;
  int numUpdates = (int)std::floor(mRemaining / tickTime);

  for(int i = 0; i < numUpdates; i++)
    { tick(memory); }

  mRemaining -= numUpdates * tickTime;
  return true;
}

void cCpu::runProgram(int addr)
{
  mProgramCounter = addr;
  LOGI("Starting progam at address: 0x%02X", addr);
}

void cCpu::tick(cMemory *memory)
{
  if(mProgramCounter >= 0)
    {
      // get instruction
      mInstructionReg = memory->access(mProgramCounter);
      mProgramCounter++;

      // execute instruction
      int reg1 = 0;
      int reg2 = 0;
      int addr = 0;
      int val = 0;
      switch(mInstructionReg)
	{
	case Instruction::LOADL:
	  reg1 = memory->access(mProgramCounter);
	  mProgramCounter++;
	  val = memory->access(mProgramCounter);
	  mProgramCounter++;
	  mRegisters[reg1] = val;
	  LOGD("LOADL: reg%d <-- %d", reg1, val);
	  break;
	case Instruction::LOADA:
	  reg1 = memory->access(mProgramCounter);
	  mProgramCounter++;
	  addr = memory->access(mProgramCounter);
	  mProgramCounter++;
	  mRegisters[reg1] = memory->access(addr);
	  LOGD("LOADA: reg%d <-- [0x%02X] (%d)", reg1, addr, val);
	  break;
	case Instruction::ADD:
	  reg1 = memory->access(mProgramCounter);
	  mProgramCounter++;
	  reg2 = memory->access(mProgramCounter);
	  mProgramCounter++;
	  val = (int)mRegisters[reg1] + (int)mRegisters[reg2];
	  if(val > maxVal)
	    {
	      LOGD("(Overflow!)");
	      mOverflow = true;
	      val = maxVal;
	    }
	  else
	    { mOverflow = false; }
	  mRegisters[reg1] = (uint8_t)val;
	  LOGD("ADD: reg%d + reg%d = %d", reg1, reg2, val);
	  break;
	case Instruction::SUBTRACT:
	  reg1 = memory->access(mProgramCounter);
	  mProgramCounter++;
	  reg2 = memory->access(mProgramCounter);
	  mProgramCounter++;
	  val = mRegisters[reg1] - mRegisters[reg2];
	  if(val < 0)
	    {
	      LOGD("(Underflow!)");
	      mUnderflow = true;
	      val = 0;
	    }
	  else
	    { mUnderflow = false; }
	  mRegisters[reg1] = val;
	  LOGD("SUBTRACT: reg%d + reg%d = %d", reg1, reg2, val);
	  break;
	case Instruction::STORE:
	  reg1 = memory->access(mProgramCounter);
	  mProgramCounter++;
	  addr = memory->access(mProgramCounter);
	  mProgramCounter++;
	  memory->set(addr, mRegisters[reg1]);
	  LOGD("STORE: reg%d (%d) --> [0x%02X]", reg1, mRegisters[reg1], addr);
	  break;
	case Instruction::JUMP:
	  addr = (int8_t)memory->access(mProgramCounter);
	  mProgramCounter += addr;
	  LOGD("JUMP: [0x%02X]", addr);
	  break;
	case Instruction::STOP:
	  mProgramCounter = -1;
	  LOGD("STOP");
	  break;
	default:
	  LOGE("Invalid instruction! --> 0x%02X", mInstructionReg);
	}
      LOGD("Counter: [0x%02X]", mProgramCounter);
    }
  else
    { LOGI("CPU TICK! (NO ACTION)"); }
}
