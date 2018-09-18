#ifndef RAY_TRACER_HPP
#define RAY_TRACER_HPP

#include "hashing.hpp"
#include "vertex.hpp"
#include "vector.hpp"
#include "geometry.hpp"

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <QOpenGLFunctions_4_3_Core>
#include <qopengl.h>

class Chunk;
class cTextureAtlas;
class ComputeShader;
class Shader;
class Frustum;
class QuadBuffer;

class RayTracer : protected QOpenGLFunctions_4_3_Core
{
public:
  RayTracer();
  ~RayTracer();
  
  bool initGL(QObject *qParent);
  void cleanupGL();
  void render(const Matrix4 &pvm, const Point3f &camPos);
  
  void setFrustum(Frustum *frustum);
  void setFrustumClip(bool on);
  void setFrustumPause();
  void setScreenSize(const Point2i &size);
  
  void load(hash_t hash, Chunk *chunk);
  void unload(hash_t hash);
  void setFog(float fogStart, float fogEnd, const Vector3f &dirScale);
  
private:
  bool mInitialized = false;
  bool mClipFrustum = true;
  Frustum *mFrustum = nullptr;
  std::unordered_set<hash_t> mFrustumRender;
  
  bool mFogChanged = false;
  float mFogStart = 0.0f;
  float mFogEnd = 0.0f;
  Vector3f mDirScale;

  Point2i mScreenSize;
  GLuint mComputeTex;
  GLuint mBlockBuffer;
  ComputeShader *mShader = nullptr;
  
  Shader *mQuadShader = nullptr;
  cTextureAtlas *mTexAtlas = nullptr;
  QuadBuffer *mQuad = nullptr;

  std::mutex mChunkLock;
  std::mutex mRenderLock;
  std::unordered_map<hash_t, Chunk*> mChunks;
  
  struct BufBlock
  {
    float type;
    float pos[3];
  };
  std::unordered_map<hash_t, GLuint> mChunkBuffers;
  std::unordered_map<hash_t, std::vector<int>> mChunkData;

  bool prepareChunkData(hash_t hash);
  void setupCompute();
};


#endif // RAY_TRACER_HPP