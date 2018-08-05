#include "glBuffer.hpp"


cGlBuffer::cGlBuffer()
{

}

cGlBuffer::~cGlBuffer()
{

}

bool cGlBuffer::create()
{
  if(!mLoaded)
    { initializeOpenGLFunctions(); }
  mLoaded = false;



  mLoaded = true;
  return true;
}
