#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "vector.hpp"
#include "meshData.hpp"
#include "matrix.hpp"

class Camera
{
public:
  Camera(const Point3f &pos = Point3f());
  
  void setProjection(float fovy, float aspect, float zNear, float zFar);
  void setView(const Vector3f &forward, const Vector3f &vertical);
  void setPos(const Point3f &pos);

  Point3f getPos() const { return mPos; }
  Matrix4 getView() const { return mViewMat; }
  Matrix4 getProjection() const { return mProjMat; }
  
  float getAspect() const { return mAspect; }
  float getFovY() const { return mFovY; }
  float getFovX() const { return mFovX; }
  float getNearZ() const { return mNearZ; }
  float getFarZ() const { return mFarZ; }

  Vector3f getEye() const { return mEye; }
  Vector3f getUp() const { return mUp; }
  Vector3f getForward() const { return mForward; }
  Vector3f getVertical() const { return mVertical; }
  Vector3f getRight() const { return mRight; }
  
  void rotate(float pitch, float yaw);
  
  bool pointInFrustum(const Point3f &p);
  bool sphereInFrustum(const Point3f &center, float radius);
  bool cubeInFrustum(const Point3f &pos, const Vector3f &size);

  MeshData makeDebugMesh();

private:
  Point3f mPos;
  
  Vector3f mForward;
  Vector3f mVertical;
  Vector3f mRight;
  Vector3f mEye;
  Vector3f mUp;
  
  float mPitch = 0.0f;
  float mFovX;
  float mFovY;
  float mAspect;
  float mNearZ;
  float mFarZ;

  
  float mNearWidth;
  float mNearHeight;
  float mFarWidth;
  float mFarHeight;

  Matrix4 mViewMat;
  Matrix4 mProjMat;
  
  struct Plane
  {
    Point3f center;
    Point3f p1;
    Point3f p2;
    Point3f opposite;
    Point3f normal;
  };
  Plane mPlanes[6];

  void updateView();
  void updateProj();
  void updateVectors();
  
  void makePlaneMesh(int planeIndex, const Vector3f &color, MeshData &mesh);
  bool pointInFrustum(const Point3f &p, const Plane &plane);
  void calcPlanes();
};


#endif // CAMERA_HPP
