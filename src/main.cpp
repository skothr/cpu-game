#include <QtGui>
#include <QApplication>
#include <QObject>

#include "mainWindow.hpp"
#include "logging.hpp"

#include <unistd.h>
#include <iostream>

#define CPU_BITS    8
#define CPU_REGS    2
#define MEM_BYTES   1024
#define OPS_PER_SEC 8

/*
static uint8_t test[] = { Instruction::LOADL, 0x00, 0x05, // load 5 into reg1
			  Instruction::LOADL, 0x01, 0x06, // load 6 into reg2
			  Instruction::ADD, 0x00, 0x01,  // reg1 + reg2
			  Instruction::STORE, 0x00, 0x20,// store at memory addr 64
			  Instruction::JUMP, (uint8_t)(-7),
			  Instruction::STOP };
*/

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  //app.setQuitOnLastWindowClosed(true);
  //QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()) );
  cMainWindow window;
  window.show();
  
  return app.exec();
  
  /*
  cCpu *cpu = new cCpu(CPU_BITS, CPU_REGS, OPS_PER_SEC);
  cMemory *mem = new cMemory(MEM_BYTES, OPS_PER_SEC);
  mem->load(0x00, test, sizeof(test));
  cpu->runProgram(0x00);
  
  cDevice dev;
  dev.addCpu(cpu);
  dev.addMemory(mem);

  dev.start();

  while(true)
    { usleep(100000); }
  return 0;
  */
}
