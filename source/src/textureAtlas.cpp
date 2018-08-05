#include "textureAtlas.hpp"
#include "logging.hpp"
#include <iostream>


cTextureAtlas::cTextureAtlas(QObject *parent, int blockSize)
  : QObject(parent), mBlockSize(blockSize)
{

}

cTextureAtlas::~cTextureAtlas()
{
  destroy();
}

#define MAX_TEXTURES 8
  
bool cTextureAtlas::create(const std::string &filePath)
{
  if(!mLoaded)
    { initializeOpenGLFunctions(); }
  else
    { destroy(); }

  glGetError();
  glGenTextures(1, &mTextureId);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, mTextureId);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
  glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_GENERATE_MIPMAP, GL_TRUE);
    
  mImage.load(QString(filePath.c_str()), "png");
  int imgW = mImage.width();
  int imgH = mImage.height();
  int xBlocks = imgW / mBlockSize;
  int yBlocks = imgH / mBlockSize;
  const unsigned char *data = mImage.bits();

  LOGD("imgW: %d, imgH: %d, xb: %d, yb: %d", imgW, imgH, xBlocks, yBlocks);
  
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, mBlockSize, mBlockSize, xBlocks * yBlocks);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, imgW);
  int i = 0;
  for(int r = 0; r < yBlocks; r++)
    {
      for(int c = 0; c < xBlocks; c++)
	{
	  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, xBlocks * r + c, mBlockSize, mBlockSize, 1, GL_BGRA, GL_UNSIGNED_BYTE, data + (r * mBlockSize * imgW + c * mBlockSize) * yBlocks / 2);
	  i++;
	  if(i >= MAX_TEXTURES)
	    { break; }
	}
      if(i >= MAX_TEXTURES)
	{ break; }
    }

  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

  mLoaded = true;
  return true;
}
void cTextureAtlas::destroy()
{
  if(mLoaded)
    {
      release();
      glDeleteTextures(1, &mTextureId);
      mLoaded = false;
    }
}
void cTextureAtlas::bind()
{
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_ARRAY, mTextureId);
}
void cTextureAtlas::release()
{
  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
