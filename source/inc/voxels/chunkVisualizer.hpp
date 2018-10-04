#ifndef MINIMAP_HPP
#define MINIMAP_HPP

#include "hashing.hpp"
#include "chunkStates.hpp"
#include "meshData.hpp"

#include <unordered_map>
#include <mutex>
#include <QOpenGLFunctions_4_3_Core>

class Shader;
class cMeshBuffer;
class QObject;


class ChunkVisualizer : protected QOpenGLFunctions_4_3_Core
{
public:
  ChunkVisualizer(const Vector2i &size);
  virtual ~ChunkVisualizer();

  void setSize(const Vector2i &size);
  void setCenter(const Point3i &center);
  void setRadius(const Vector3i &radius);
  void setState(hash_t hash, ChunkState state);
  void unload(hash_t hash);

  void setEyeAngle(const Vector3f &eyeAngle);
  Vector3f getEyeAngle() const;
  
  bool initGL(QObject *qParent);
  void cleanupGL();
  void render();
  
private:
  bool mInitialized = false;
  Shader *mShader = nullptr;
  cMeshBuffer *mMesh = nullptr;
  MeshData mMeshData;
  bool mNewData = false;
  std::unordered_map<hash_t, ChunkState> mChunks;
  Vector2i mSize;
  Point3i mCenter;
  Vector3i mRadius;

  Vector3f mEyeAngle = Vector3f{0,0,-1};

  std::mutex mLock;

  void updateMesh();
};

#endif // MINIMAP_HPP
