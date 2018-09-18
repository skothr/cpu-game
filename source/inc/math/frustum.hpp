#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP


#include "vector.hpp"
#include "geometry.hpp"

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
  
  bool pointInside(const Point3f &p, const Matrix4 &pv);
  bool sphereInside(const Point3f &center, float radius);
  bool cubeInside(const Point3f &center, const Vector3f &size, const Matrix4 &pv);
  
private:
  struct Plane
  {
    Vector3f norm;
    float scalar;
  };

  Plane mPlanes[6];
  
  float mFov;
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

  void calcPlanes();
};

#endif // FRUSTUM_HPP
