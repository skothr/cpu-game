#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP


#include "vector.hpp"
#include "geometry.hpp"
#include "meshData.hpp"

class Frustum
{
public:
  Frustum();

  void setProjection(float fov, float aspect, float zNear, float zFar);
  void setView(const Vector3f &forward, const Vector3f &up);
  void setPos(const Point3f &center);
  void rotate(float pitch, float yaw);

  Point3f getPos() const;
  Vector3f getEye() const;
  Vector3f getEyeRight() const;
  Vector3f getEyeUp() const;
  Point3f getCenter() const;
  Matrix4 getView() const;
  Matrix4 getProjection() const;

  float getAspect() const;
  float getFov() const;

  Vector3f getForward() const { return mForward; }
  Vector3f getUp() const { return mUp; }
  Vector3f getRight() const { return mRight; }
  
  bool pointInside(const Point3f &p);
  bool sphereInside(const Point3f &center, float radius);
  bool cubeInside(const Point3f &pos, const Vector3f &size);

  MeshData makeDebugMesh();
  
private:
  struct Plane
  {
    Point3f center;
    Point3f p1;
    Point3f p2;
    Point3f opposite;
    Point3f normal;
  };
  Plane mPlanes[6];
  
  float mFovX;
  float mFovY;
  float mAspect;
  float mNearZ;
  float mFarZ;

  float mNearWidth;
  float mNearHeight;
  float mFarWidth;
  float mFarHeight;

  Point3f mCenter;
  Vector3f mForward;
  Vector3f mUp;
  Vector3f mRight;
  
  float mPitch;
  void makePlaneMesh(int planeIndex, const Vector3f &color, MeshData &mesh);
  bool pointInside(const Point3f &p, const Plane &plane);
  void calcPlanes();
};

#endif // FRUSTUM_HPP
