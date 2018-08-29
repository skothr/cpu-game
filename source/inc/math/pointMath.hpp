#ifndef POINT_MATH_HPP
#define POINT_MATH_HPP

#include "vector.hpp"

inline bool pointInRange(const Point3i &p, const Point3i &min, const Point3i &max)
{
  return (p[0] >= min[0] && p[0] <= max[0] &&
          p[1] >= min[1] && p[1] <= max[1] &&
          p[2] >= min[2] && p[2] <= max[2] );
}

#endif // POINT_MATH_HPP
