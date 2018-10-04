#include "camera.hpp"
#include "geometry.hpp"
#include "miscMath.hpp"
#include <iostream>

Camera::Camera(const Point3f &pos)
  : mPos(pos)
{

}

void Camera::setProjection(float fovy, float aspect, float zNear, float zFar)
{
  mFovY = fovy;
  mFovX = 2.0f * atan(tan(mFovY / 2.0f) * aspect);
  mAspect = aspect;
  mNearZ = zNear;
  mFarZ = zFar;
  
  const float tanY = tan(mFovY/2.0f);
  const float tanX = tan(mFovX/2.0f);
  
  mNearHeight = mNearZ * tanY;
  mNearWidth = mNearZ * tanX;
  mFarHeight = mFarZ * tanY;
  mFarWidth = mFarZ * tanX;
  
  updateProj();
  calcPlanes();
}

void Camera::setView(const Vector3f &forward, const Vector3f &vertical)
{
  mForward = forward.normalized();
  mVertical = vertical.normalized();
  
  updateVectors();
  updateView();
  calcPlanes();
}

void Camera::setPos(const Point3f &pos)
{
  mPos = pos;
  
  updateView();
  calcPlanes();
}

void Camera::rotate(float pitch, float yaw)
{
  mForward = mForward.rotated(mVertical, degreesToRadians(yaw)).normalized();
  mPitch = std::min(1.0f, std::max(-1.0f, mPitch - pitch/100.0f));
  updateVectors();
  updateView();
  calcPlanes();
}

void Camera::updateView()
{
  QMatrix4x4 v;
  v.lookAt(toQt(mPos), toQt(mPos + mEye), toQt(mUp));
  mViewMat = Matrix4(v);
}
void Camera::updateProj()
{
  QMatrix4x4 p;
  p.perspective(radiansToDegrees(mFovY), mAspect, mNearZ, mFarZ);
  mProjMat = Matrix4(p);
}
void Camera::updateVectors()
{ // everything is based off of mForward, mVertical, and mPitch.
  mRight = crossProduct(mForward, mVertical).normalized();
  if(mPitch >= 0.0f)
    {
      mEye = lerp(mForward, mVertical, mPitch).normalized();
      mUp = lerp(mVertical, -mForward, mPitch).normalized();
    }
  else
    {
      mEye = lerp(mForward, -mVertical, -mPitch).normalized();
      mUp = lerp(mVertical, mForward, -mPitch).normalized();
    }
}

void Camera::calcPlanes()
{
  const Point3f nearCenter = mPos + mEye * mNearZ;
  const Point3f farCenter = mPos + mEye * mFarZ;

  const Vector3f nearOffsetW = mRight*mNearWidth;
  const Vector3f nearOffsetH = mUp*mNearHeight;
  const Vector3f farOffsetW = mRight*mFarWidth;
  const Vector3f farOffsetH = mUp*mFarHeight;
  
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
  // // front (near)
  // mPlanes[4] = { nbl, nbr, ntl, ntr,
  //                crossProduct((nbr - nbl), (ntl - nbl)).normalized() };
  // // back (far)
  // mPlanes[5] = { fbr, fbl, ftr, ftl,
  //                crossProduct((fbl - fbr), (ftr - fbr)).normalized() };
}

bool Camera::sphereInFrustum(const Point3f &center, float radius)
{
  return false;
}

bool Camera::pointInFrustum(const Point3f &p)
{
  for(int i = 0; i < 4; i++)
    {
      if(!pointInFrustum(p, mPlanes[i]))
        { return false; }
    }
  return true;
}

bool Camera::pointInFrustum(const Point3f &p, const Camera::Plane &plane)
{
  return plane.normal.dot((plane.center - p).normalized()) >= 0.0;
}

bool Camera::cubeInFrustum(const Point3f &pos, const Vector3f &size)
{
  const Point3f maxP = pos + size;
  for(int i = 0; i < 4; i++) // don't need to check near/far planes
    { // check if all corners are outside plane
      if(!pointInFrustum(pos, mPlanes[i]) &&
         !pointInFrustum({maxP[0], pos[1], pos[2]}, mPlanes[i]) &&
         !pointInFrustum({pos[0], maxP[1], pos[2]}, mPlanes[i]) &&
         !pointInFrustum({maxP[0], maxP[1], pos[2]}, mPlanes[i]) &&
         !pointInFrustum({pos[0], pos[1], maxP[2]}, mPlanes[i]) &&
         !pointInFrustum({maxP[0], pos[1], maxP[2]}, mPlanes[i]) &&
         !pointInFrustum({pos[0], maxP[1], maxP[2]}, mPlanes[i]) &&
         !pointInFrustum(maxP, mPlanes[i]) )
        { return false; }
    }
  return true;
}

void Camera::makePlaneMesh(int planeIndex, const Vector3f &color, MeshData &mesh)
{
  Plane &p = mPlanes[planeIndex];

  const Vector3f offset = mEye * 0.01;
  
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
MeshData Camera::makeDebugMesh()
{
  MeshData data;
  makePlaneMesh(0, Vector3f{1,0,0}, data); // left   -- red
  makePlaneMesh(1, Vector3f{0,1,0}, data); // right  -- green
  makePlaneMesh(2, Vector3f{1,1,0}, data); // top    -- yellow
  makePlaneMesh(3, Vector3f{0,1,1}, data); // bottom -- cyan
  //makePlaneMesh(4, Vector3f{1,1,1}, data); // near   -- white
  //makePlaneMesh(5, Vector3f{1,0,1}, data); // far    -- magenta
  return data;
}
