#include "frustum.hpp"
#include "miscMath.hpp"

#include <QMatrix4x4>
#include <cmath>

Frustum::Frustum()
{

}


void Frustum::setProjection(float fovy, float aspect, float zNear, float zFar)
{
  mFovY = fovy*M_PI/180.0;
  mFovX = 2*atan(tan(mFovY/2.0f)*aspect);
  mAspect = aspect;
  mNearZ = zNear;
  mFarZ = zFar;
  
  const float tanY = tan(mFovY*0.5f);
  const float tanX = tan(mFovX*0.5f);
  
  mNearHeight = mNearZ * tanY;
  mFarHeight = mFarZ * tanY;
  mNearWidth = mNearZ * tanX;
  mFarWidth = mFarZ * tanX;
  calcPlanes();
}

void Frustum::setView(const Vector3f &forward, const Vector3f &up)
{
  mForward = forward.normalized();
  mUp = up.normalized();
  mRight = crossProduct(mForward, mUp).normalized();
  calcPlanes();
}

void Frustum::setPos(const Point3f &center)
{
  mCenter = center;
  calcPlanes();
}

void Frustum::rotate(float pitch, float yaw)
{
  QMatrix4x4 rot;
  rot.rotate(yaw, toQt(mUp));
  mForward = (mForward.asVector()*Matrix4(rot).transposed()).normalized();
  mRight = (crossProduct(mForward, mUp)).normalized();

  mPitch += pitch/-100.0f;
  if(mPitch > 1.0f)
    { mPitch = 1.0f; }
  else if(mPitch < -1.0f)
    { mPitch = -1.0f; }
  calcPlanes();
}


float Frustum::getAspect() const
{
  return mAspect;
}

float Frustum::getFov() const
{
  return mFovY*180.0/M_PI;
}

Vector3f Frustum::getEye() const
{
  if(mPitch >= 0.0f)
    { return lerp(mForward, mUp, mPitch).normalized(); }
  else
    { return lerp(mForward, -mUp, -mPitch).normalized(); }
}
Vector3f Frustum::getEyeRight() const
{
  return mRight;
}
Vector3f Frustum::getEyeUp() const
{
  if(mPitch >= 0.0f)
    { return lerp(mUp, -mForward, mPitch).normalized(); }
  else
    { return lerp(mUp, mForward, -mPitch).normalized(); }
}
Point3f Frustum::getPos() const
{
  return mCenter; 
}

Matrix4 Frustum::getView() const
{
  QMatrix4x4 v;
  v.lookAt(toQt(mCenter), toQt(mCenter + getEye()), toQt(getEyeUp()));
  return Matrix4(v);
}

Matrix4 Frustum::getProjection() const
{
  QMatrix4x4 p;
  p.perspective(mFovY*180.0/M_PI, mAspect, mNearZ, mFarZ);
  return p;
}

void Frustum::calcPlanes()
{
  const Vector3f eye = getEye();
  const Vector3f up = getEyeUp();

  const Point3f nearCenter = mCenter + eye * mNearZ;
  const Point3f farCenter = mCenter + eye * mFarZ;

  const Vector3f nearOffsetW = mRight*mNearWidth;
  const Vector3f nearOffsetH = up*mNearHeight;
  const Vector3f farOffsetW = mRight*mFarWidth;
  const Vector3f farOffsetH = up*mFarHeight;
  
  const Point3f nbl = nearCenter - nearOffsetW - nearOffsetH;
  const Point3f nbr = nearCenter + nearOffsetW - nearOffsetH;
  const Point3f ntl = nearCenter - nearOffsetW + nearOffsetH;
  const Point3f ntr = nearCenter + nearOffsetW + nearOffsetH;
  
  const Point3f fbl = farCenter - farOffsetW - farOffsetH;
  const Point3f fbr = farCenter + farOffsetW - farOffsetH;
  const Point3f ftl = farCenter - farOffsetW + farOffsetH;
  const Point3f ftr = farCenter + farOffsetW + farOffsetH;

  // left
  mPlanes[0] = { nbl, ntl, fbl, ftl,
                 crossProduct((ntl - nbl), (fbl - nbl)).normalized() };
  // right
  mPlanes[1] = { nbr, fbr, ntr, ftr,
                 crossProduct((fbr - nbr), (ntr - nbr)).normalized() };
  // top
  mPlanes[2] = { ntl, ntr, ftl, ftr,
                 crossProduct((ntr - ntl), (ftl - ntl)).normalized() };
  // bottom
  mPlanes[3] = { nbl, fbl, nbr, fbr,
                 crossProduct((fbl - nbl), (nbr - nbl)).normalized() };
  // front (near)
  mPlanes[4] = { nbl, nbr, ntl, ntr,
                 crossProduct((nbr - nbl), (ntl - nbl)).normalized() };
  // back (far)
  mPlanes[5] = { fbr, fbl, ftr, ftl,
                 crossProduct((fbl - fbr), (ftr - fbr)).normalized() };
}

bool Frustum::sphereInside(const Point3f &center, float radius)
{
  return false;
}

bool Frustum::pointInside(const Point3f &p, const Frustum::Plane &plane)
{
float planeVal = plane.normal.dot((plane.center - p).normalized());
  return planeVal >= 0.0;
}

bool Frustum::cubeInside(const Point3f &pos, const Vector3f &size)
{
  const Point3f maxP = pos + size;
  for(int i = 0; i < 6; i++)
    { // check if all corners are outside plane
      if(!pointInside(pos, mPlanes[i]) &&
         !pointInside({maxP[0], pos[1], pos[2]}, mPlanes[i]) &&
         !pointInside({pos[0], maxP[1], pos[2]}, mPlanes[i]) &&
         !pointInside({maxP[0], maxP[1], pos[2]}, mPlanes[i]) &&
         !pointInside({pos[0], pos[1], maxP[2]}, mPlanes[i]) &&
         !pointInside({maxP[0], pos[1], maxP[2]}, mPlanes[i]) &&
         !pointInside({pos[0], maxP[1], maxP[2]}, mPlanes[i]) &&
         !pointInside(maxP, mPlanes[i]) )
        { return false; }
    }
  return true;
}

void Frustum::makePlaneMesh(int planeIndex, const Vector3f &color, MeshData &mesh)
{
  Plane &p = mPlanes[planeIndex];

  const Vector3f offset = getEye() * 0.01;
  
  int numVert = mesh.vertices().size();
  mesh.vertices().emplace_back(p.center + offset, color, Point2i{0,0} );
  mesh.vertices().emplace_back(p.p1 + offset, color, Point2i{0,0} );
  mesh.vertices().emplace_back(p.p2 + offset, color, Point2i{0,0} );
  mesh.vertices().emplace_back(p.opposite + offset, color, Point2i{0,0} );
  
  mesh.indices().push_back(numVert + 0);
  mesh.indices().push_back(numVert + 1);
  mesh.indices().push_back(numVert + 2);
  mesh.indices().push_back(numVert + 2);
  mesh.indices().push_back(numVert + 1);
  mesh.indices().push_back(numVert + 3);
}
MeshData Frustum::makeDebugMesh()
{
  MeshData data;
  Vector3f lColor{1,0,0};
  Vector3f rColor{0,1,0};
  Vector3f tColor{1,1,0};
  Vector3f bColor{0,1,1};
  Vector3f nColor{1,1,1};
  Vector3f fColor{1,0,1};

  makePlaneMesh(0, lColor, data);
  makePlaneMesh(1, rColor, data);
  makePlaneMesh(2, tColor, data);
  makePlaneMesh(3, bColor, data);
  makePlaneMesh(4, nColor, data);
  makePlaneMesh(5, fColor, data);
  
  return data;
}
