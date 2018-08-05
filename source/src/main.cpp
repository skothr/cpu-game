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
              //case 'g':
	      //genWorldFileOut = argv[++i];
	      //break;
	    }
	}
    }

  if(test)
    {

      cPerlinNoise n(123);

      for(int x = -1000; x < 1000; x += 10)
        for(int y = -1000; y < 1000; y += 10)
          for(int z = -1000; z < 1000; z += 10)
            { std::cout << (double)n.noise(x/1000.0, y/1000.0, z/1000.0)*1000.0 << "  "; }
      std::cout << "\n\n";
      return 0;
      
      /*
      cMmapFile f;
      LOGD("Opening...");
      if(!f.open("res/test.bin", cMmapFile::INPUT | cMmapFile::OUTPUT))
	{ exit(1); }

      LOGD("Reading...");
      char *buf = new char[32];

      f.read(0, buf, 8);

      LOGD("Done");
      */
    }

  /*
  if(genWorldFileOut != "")
    {
      cChunk chunk(Point3i{0,0,0});
      for(int x = 0; x < cChunk::sizeX; x++)
	{
	  for(int y = 0; y < cChunk::sizeY; y++)
	    {
	      for(int z = 0; z < cChunk::sizeZ; z++)
		{
		  if(z == 0)
		    {
		      LOGD("Setting %d,%d,%d", x, y, z);
		      chunk.set(x, y, z, block_t::DIRT);
		    }
		}
	    }
	}

      LOGI("Chunk Size: %dx%dx%d", cChunk::sizeX, cChunk::sizeY, cChunk::sizeZ);
      LOGI("Num chunks: %d", 1);
      FileHeader header{{0,0,0,1}, cBlock::dataSize, terrain_t::PERLIN, 1};
      writeWorldFile(genWorldFileOut, header, { &chunk });
      return 0;
    }
  else
    {
  */

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
