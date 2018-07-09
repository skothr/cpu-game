#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <vector>
#include <QMatrix4x4>
#include <cstdint>
#include "model.hpp"

class cShader;

enum class block_t : uint8_t
  {
   NONE = 0,
   FLOOR,
   DEVICE,
   CPU,
   MEMORY,
   LIGHT,
   
   COUNT
  };


class cBlockUpdate
{
public:
  cBlockUpdate();
private:
};

class cBlock
{
public:
  cBlock() { }
  cBlock(block_t t)
    : type(t)
  { }

  /*
  virtual void step(const Point3i *pos, cOctree *blocks)
  {
    switch(type)
      {
      case block_t::FLOOR:
	break;
      case block_t::DEVICE:
	{
	  cBlock *left = blocks->get(*pos + Vector3i{-1,0,0});
	  cBlock *right = blocks->get(*pos + Vector3i{1,0,0});
	  //if(left && 
	  break;
	}
      case block_t::CPU:
	break;
      case block_t::MEMORY:
	break;
      default:
	break;
      }
  }
  */
  
  block_t type = block_t::NONE;
  
  //float light = 0.0f;
  //float oxygen = 1.0f;
};


#define CHUNK_DIM 16

class cBlockChunk
{
public:
  cBlockChunk();
  cBlock mData[CHUNK_DIM*CHUNK_DIM*CHUNK_DIM];

  int getIndex(int x, int y, int z)
  {
    return z*CHUNK_DIM*CHUNK_DIM + y*CHUNK_DIM + x;
  }
};



#endif // BLOCK_HPP
