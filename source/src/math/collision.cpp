#include "collision.hpp"

#include <iostream>
#include <algorithm>
#include <iomanip>

cBoundingBox::cBoundingBox()
{

}

cBoundingBox::cBoundingBox(const Point3f &center, const Vector3f &size)
  : mCenter(center), mSize(size), mMin(center - size*0.5f),
    mMax(center + size*0.5f)
{
  
}


bool cBoundingBox::collides(const cBoundingBox &b) const
{
  return ((mMin[0] < b.mMax[0] && mMax[0] > b.mMin[0]) &&
	  (mMin[1] < b.mMax[1] && mMax[1] > b.mMin[1]) &&
	  (mMin[2] < b.mMax[2] && mMax[2] > b.mMin[2]) );
}
bool cBoundingBox::collidesEdge(const cBoundingBox &b) const
{
  return ((mMin[0] <= b.mMax[0] && mMax[0] >= b.mMin[0]) &&
	  (mMin[1] <= b.mMax[1] && mMax[1] >= b.mMin[1]) &&
	  (mMin[2] <= b.mMax[2] && mMax[2] >= b.mMin[2]) );
}

static int sign(float f)
{
  return (f > 0 ? 1 : (f < 0 ? -1 : 0));
}

static float intersect(float rayP, float faceP, float rayDir)
{
  //if(rayDir == 0.0f)
  //  { return -1000.0f; }
  float t = (faceP - rayP) / rayDir;
  if(t < -1.0f || t > 1.0f)// || t > 1.0f)// || t < -1.0f)
    {
      return -1000.0f;
    }
  return t;
}

// dir is direction of this box's movement.
// returns correction in form of t value for dir (p + dir*t)
Vector3f cBoundingBox::correction(const cBoundingBox &b, const Vector3f &dir, const Vector3i &ground) const
{
  float tx = intersect(dir[0] < 0.0f ? mMin[0] - dir[0] : mMax[0] - dir[0],
		       dir[0] < 0.0f ? b.mMax[0] : b.mMin[0],
		       dir[0] );
  float ty = intersect(dir[1] < 0.0f ? mMin[1] - dir[1] : mMax[1] - dir[1],
		       dir[1] < 0.0f ? b.mMax[1] : b.mMin[1],
		       dir[1] );
  float tz = intersect(dir[2] < 0.0f ? mMin[2] - dir[2] : mMax[2] - dir[2],
		       dir[2] < 0.0f ? b.mMax[2] : b.mMin[2],
		       dir[2] );
  
  //std::cout << "TX: " << tx << ", TY: " << ty << ", TZ: " << tz << "\n";

  Vector3f result{0.0f, 0.0f, 0.0f};
  if(tx > -1000.0f &&  (!ground[1] || tx <= 0.0f) && (ground[0] || (tx > ty && tx > tz)))
    {
      result += Vector3f{(1.0f - tx)*dir[0], 0.0f, 0.0f};
    }
  if(ty > -1000.0f && (!ground[0] || ty <= 0.0f) && (ground[1] || (ty > tx && ty > tz)))
    {
      result += Vector3f{0.0f, (1.0f - ty)*dir[1], 0.0f};
    }
  if(tz > -1000.0f && ((!ground[0] && !ground[1]) || tz <= 0.0f) && (ground[2] || (tz > tx && tz > ty)))
    {
      result += Vector3f{0.0f, 0.0f, (1.0f - tz)*dir[2]};
    }
  
  return result;
}

bool cBoundingBox::contains(const Point3f &p) const
{
  return ((p[0] > mMin[0] && p[0] < mMax[0]) &&
	  (p[1] > mMin[1] && p[1] < mMax[1]) &&
	  (p[2] > mMin[2] && p[2] < mMax[2]) );
}
bool cBoundingBox::containsEdge(const Point3f &p) const
{
  return ((p[0] >= mMin[0] && p[0] <= mMax[0]) &&
	  (p[1] >= mMin[1] && p[1] <= mMax[1]) &&
	  (p[2] >= mMin[2] && p[2] <= mMax[2]) );
}
bool cBoundingBox::onEdge(const Point3f &p) const
{
  return (((p[0] == mMin[0] || p[0] == mMax[0]) &&
	   (p[1] > mMin[1] && p[1] < mMax[1]) &&
	   (p[2] > mMin[2] && p[2] < mMax[2])) ||
	  ((p[1] == mMin[1] || p[1] == mMax[1]) &&
	   (p[0] > mMin[0] && p[0] < mMax[0]) &&
	   (p[2] > mMin[2] && p[2] < mMax[2])) ||
	  ((p[2] == mMin[2] || p[2] == mMax[2]) &&
	   (p[1] > mMin[1] && p[1] < mMax[1]) &&
	   (p[0] > mMin[0] && p[0] < mMax[0])));
}

Point3f cBoundingBox::maxPoint() const { return mMax; }
Point3f cBoundingBox::minPoint() const { return mMin; }


Point3f cBoundingBox::center() const
{ return mCenter; }
Point3f cBoundingBox::size() const
{ return mSize; }

void cBoundingBox::setCenter(const Point3f &center)
{
  mCenter = center;
  mMax = mCenter + mSize*0.5f;
  mMin = mCenter - mSize*0.5f;
}
void cBoundingBox::setSize(const Vector3f &size)
{
  mSize = size;
  mMax = mCenter + mSize*0.5f;
  mMin = mCenter - mSize*0.5f;
}
void cBoundingBox::move(const Vector3f &dPos)
{
  setCenter(mCenter + dPos);
}


cCollidable::cCollidable(const Point3f &center, const Vector3f &size)
  : mBox(center, size)
{

}
