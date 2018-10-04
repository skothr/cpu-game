#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <cmath>

inline float degreesToRadians(float dAngle)
{ return dAngle * M_PI / 180.0f; }
inline float radiansToDegrees(float rAngle)
{ return rAngle * 180.0f / M_PI; }


#endif // GEOMETRY_HPP
