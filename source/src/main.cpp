#include <QtGui>
#include <QApplication>
#include <QObject>

#include "mainWindow.hpp"
#include "logging.hpp"

#include "chunkManager.hpp"
#include "chunk.hpp"

#include <unistd.h>
#include <iostream>
#include "mmapFile.hpp"


int main(int argc, char *argv[])
{
  int numThreads = 1;
  bool test = false;
  std::string worldName = "";
  uint32_t seed = 0;
  bool printWorld = false;
  //std::string genWorldFileOut = "";
  for(int i = 0; i < argc; i++)
    {
      if(argv[i][0] == '-')
	{
	  switch(argv[i][1])
	    {
	    case 'n':
	      numThreads = std::atoi(argv[++i]);
	      break;
	    case 'w':
	      worldName = argv[++i];
	      break;
	    case 's':
	      seed = std::atoi(argv[++i]);
	      break;
	      
	      // tools:
	    case 't':
	      test = true;
	      break;
            case 'p':
              printWorld = true;
              break;
	    }
	}
    }

  if(test)
    { // test stuff goes in here
      
      return 0;
    }
  QApplication app(argc, argv);
  cMainWindow window(nullptr, numThreads, worldName, seed);
  window.resize(1080, 1080);
  window.show();
  return app.exec();
}



// #define CPU_BITS    8
// #define CPU_REGS    2
// #define MEM_BYTES   1024
// #define OPS_PER_SEC 8
// uint8_t test[] = { Instruction::LOADL, 0x00, 0x05, // load 5 into reg1
// 			  Instruction::LOADL, 0x01, 0x06, // load 6 into reg2
// 			  Instruction::ADD, 0x00, 0x01,  // reg1 + reg2
// 			  Instruction::STORE, 0x00, 0x20,// store at memory addr 64
// 			  Instruction::JUMP, (uint8_t)(-7),
// 			  Instruction::STOP };
