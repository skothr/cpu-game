#ifndef COLLISION_HPP
#define COLLISION_HPP

#include "vector.hpp"


class cBoundingBox
{
public:
  cBoundingBox();
  cBoundingBox(const Point3f &pos, const Vector3f &size);

  bool collides(const cBoundingBox &b) const;
  bool collidesEdge(const cBoundingBox &b) const;
  bool contains(const Point3f &p) const;
  bool containsEdge(const Point3f &p) const;
  bool onEdge(const Point3f &p) const;

  Vector3f correction(const cBoundingBox &b, const Vector3f &dir, const Vector3i &onGround) const;

  Point3f pos() const;
  Point3f size() const;
  Point3f maxPoint() const;
  Point3f minPoint() const;

  void setPos(const Point3f &pos);
  void setSize(const Vector3f &size);

  void move(const Vector3f &dPos);
  
private:
  Point3f mPos;
  Point3f mMax;
  Vector3f mSize;
};

class cCollidable
{
public:
  cCollidable(const Point3f &pos, const Vector3f &size);
  
protected:
  cBoundingBox mBox;
};

#endif // COLLISION_HPP
