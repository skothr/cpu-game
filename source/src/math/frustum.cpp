#include "frustum.hpp"

#include <QMatrix4x4>
#include <cmath>

Frustum::Frustum()
{

}


void Frustum::setProjection(float fovy, float aspect, float zNear, float zFar)
{
  mFov = fovy;
  mAspect = aspect;
  mNearZ = zNear;
  mFarZ = zFar;

  mNearHeight = 2.0f * mNearZ * atan(fovy/2.0f);
  mFarHeight = 2.0f * mFarZ * atan(fovy/2.0f);
  mNearWidth = mNearHeight / mAspect;
  mFarWidth = mFarHeight / mAspect;
  calcPlanes();

  std::cout << "FOV: " << mFov << " | ASPECT: " << mAspect << " | NEAR: " << mNearZ << " | FAR: " << mFarZ << "\n";
  std::cout << "  --> nHeight: " << mNearHeight << ", nWidth: " << mNearWidth << ", fHeight: " << mFarHeight << ", fWidth: " << mFarWidth << "\n";
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

Vector3f Frustum::getEye() const
{
  return (mForward*(1.0 - std::abs(mPitch)) + mUp * (mPitch)).normalized();
}
Point3f Frustum::getPos() const
{
  return mCenter;
}

Matrix4 Frustum::getView() const
{
  QMatrix4x4 v;
  v.lookAt(toQt(mCenter), toQt(mCenter + getEye()),
           toQt((std::abs(mPitch) == 1.0f ? mForward * -mPitch : mUp)));
  return Matrix4(v);
}

Matrix4 Frustum::getProjection() const
{
  QMatrix4x4 p;
  p.perspective(mFov, mAspect, mNearZ, mFarZ);
  return p;
}

void Frustum::calcPlanes()
{
  Matrix4 m = getProjection()*getView();
  m = m.transposed();
  
  // near and far planes
  // left
  Vector3f n = Vector3f({m.at(0,3) - m.at(0,0), m.at(1,3) - m.at(1,0), m.at(2,3) - m.at(2,0)});
  float l = n.length();
  mPlanes[0] = {n/l, (m.at(3,3) - m.at(3,0))/l};
  // right
  n = Vector3f({m.at(0,3) + m.at(0,0), m.at(1,3) + m.at(1,0), m.at(2,3) + m.at(2,0)});
  l = n.length();
  mPlanes[1] = {n/l, (m.at(3,3) + m.at(3,0))/l};
  // top
  n = Vector3f({m.at(0,3) + m.at(0,1), m.at(1,3) + m.at(1,1), m.at(2,3) + m.at(2,1)});
  l = n.length();
  mPlanes[2] = {n/l, (m.at(3,3) + m.at(3,1))/l};
  // bottom
  n = Vector3f({m.at(0,3) - m.at(0,1), m.at(1,3) - m.at(1,1), m.at(2,3) - m.at(2,1)});
  l = n.length();
  mPlanes[3] = {n/l, (m.at(3,3) - m.at(3,1))/l};
  // back
  n = Vector3f({m.at(0,3) - m.at(0,2), m.at(1,3) - m.at(1,2), m.at(2,3) - m.at(2,2)});
  l = n.length();
  mPlanes[4] = {n/l, (m.at(3,3) - m.at(3,2))/l};
  // front
  n = Vector3f({m.at(0,3) + m.at(0,2), m.at(1,3) + m.at(1,2), m.at(2,3) + m.at(2,2)});
  l = n.length();
  mPlanes[5] = {n/l, (m.at(3,3) + m.at(3,2))/l};
}



bool Frustum::pointInside(const Point3f &p, const Matrix4 &pv)
{
  Vector<float, 4> pos({p[0]-mCenter[0], p[1]-mCenter[1], p[2]-mCenter[2], 1.0f});
  Vector<float, 4> v = pv*pos;

  return (v[0] > -v[3] && v[0] < v[3] &&
          v[1] > -v[3] && v[1] < v[3] &&
          v[2] > 0 && v[2] < v[3] );
}

bool Frustum::sphereInside(const Point3f &center, float radius)
{
  return false;
}

bool Frustum::cubeInside(const Point3f &center, const Vector3f &size, const Matrix4 &pv)
{
  bool inside = false;
  /*
  Point<float, 4> sp{center[0], center[1], center[2], 1.0};
  sp = pv * sp;

  if(sp[0] > -sp[3] && sp[0] < sp[3] &&
     sp[1] > -sp[3] && sp[1] < sp[3] &&
     sp[2] > 0 && sp[2] < sp[3] )
  */

  /*
  for(int x = 0; x <=1; x++)
    for(int y = 0; y <=1; y++)
      for(int z = 0; z <=1; z++)
        {
          inside |= ((center + size*Vector3f{x,y,z}) - mCenter).dot(getEye() - mRight) > 0;
        }
  return inside;  
*/  
  for(int i = 0; i < 6; i++)
    {
      Vector<float, 4> v = (getView()*Vector<float, 4>({center[0], center[1], center[2], 0.0f}));
      float d = Vector3f{v[0], v[1], v[2]}.dot(mPlanes[i].norm);
      float r = (size[0] * std::abs(mPlanes[i].norm[0]) +
                 size[1] * std::abs(mPlanes[i].norm[1]) +
                 size[2] * std::abs(mPlanes[i].norm[2]) );

      float dpr = d + r;

      if(dpr < -mPlanes[i].scalar)
        { return false; }
    }
  return true;
}
