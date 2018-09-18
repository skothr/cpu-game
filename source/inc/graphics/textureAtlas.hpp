#ifndef TEXTURE_ATLAS_HPP
#define TEXTURE_ATLAS_HPP

#include <QOpenGLFunctions_4_3_Core>
#include <QImage>
#include <QObject>

#define ATLAS_BLOCK_SIZE 64
#define ATLAS_NUM_BLOCKS 8

class cTextureAtlas : public QObject, protected QOpenGLFunctions_4_3_Core
{
  Q_OBJECT
public:
  cTextureAtlas(QObject *parent, int blockSize);
  ~cTextureAtlas();

  bool setAtlas();
  
  bool create(const std::string &filePath);
  void destroy();
  
  void bind(int texNum = 0);
  void release();

  
  
private:
  GLuint mTextureId = 0;
  bool mLoaded = false;
  int mBlockSize;
  QImage mImage;
};


#endif // TEXTURE_ATLAS_HPP
