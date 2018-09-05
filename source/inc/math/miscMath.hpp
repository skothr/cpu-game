#ifndef MISC_HPP
#define MISC_HPP

#include "vector.hpp"
#include <functional>
#include <vector>
// inline int sumBits(uint32_t i)
// {
//   i = i - ((i >> 1) & 0x55555555);
//   i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
//   return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
// }



inline float lerp(float x0, float x1, float a)
{
  return (1.0f - a) * x0 + (a * x1);
}

static Vector3i gCenterLoopInit;
static std::vector<Point3i> gCenterLoopPoints;
static void initCenterLoop(const Vector3i &maxRadius)
{
  if(gCenterLoopInit != maxRadius)
    {
      gCenterLoopPoints.clear();
      gCenterLoopPoints.reserve((2*maxRadius[0]+1)*(2*maxRadius[1]+1)*(2*maxRadius[2]+1));
      
      for(int x = -maxRadius[0]; x <= maxRadius[0]; x++)
        for(int y = -maxRadius[1]; y <= maxRadius[1]; y++)
          for(int z = -maxRadius[2]; z <= maxRadius[2]; z++)
            {
              gCenterLoopPoints.push_back(Point3i{x,y,z});
            }
      std::sort(gCenterLoopPoints.begin(), gCenterLoopPoints.end(),
                [&gCenterLoopPoints](const Point3i &p1,const Point3i &p2)
                {
                  return ((p1[0]*p1[0] + p1[1]*p1[1] + p1[2]*p1[2]) <
                          (p2[0]*p2[0] + p2[1]*p2[1] + p2[2]*p2[2]) );
                });
      gCenterLoopInit = maxRadius;
    }
}

inline const std::vector<Point3i>& getCenterDistPoints(const Point3i &center,
                                                       const Point3i &maxRadius )
{
  initCenterLoop(maxRadius);
  return gCenterLoopPoints;
}


typedef std::function<bool(const Point3i &pos)> centerLoopCallback_t;
inline int loopFromCenter(const Point3i &center, const Point3i &maxRadius,
                          const centerLoopCallback_t &callback, bool reverse = false )
{
  int num = 0;
  initCenterLoop(maxRadius);
  for(auto &p : gCenterLoopPoints)
    {
      callback(center + p);
      num++;
    }
  
  /*
  int maxR = std::max(maxRadius[0], std::max(maxRadius[1], maxRadius[2]));
  Point3i dimR;
  Vector<bool, 3> skip{false, false, false};
  
  Point3i radius;
  int start = (reverse ? maxR : 0);
  int end = (reverse ? 0 : maxR);
  int step = (reverse ? -1 : 1);

    
  for(int z = -maxRadius[2]; z <= maxRadius[2]; z++)
    {
      callback(center + Point3i{0, 0, z});
      callback(center + Point3i{0, 0, z});
    }
  start++;
  
  for(int r = start; (reverse ? r >= end : r <= end); r += step)
    {
      radius[0] = std::min(r, maxRadius[0]);
      radius[1] = std::min(r, maxRadius[1]);
      radius[2] = std::min(r, maxRadius[2]);

      for(int x = -radius[0]; x < radius[0]; x++)
        for(int z = -radius[2]; z <= radius[2]; z++)
          {
            callback(center + Point3i{x+1, radius[1], z});
            callback(center + Point3i{x, -radius[1], z});
          }
      for(int y = -radius[1]; y < radius[1]; y++)
        for(int z = -radius[2]; z <= radius[2]; z++)
          {
            callback(center + Point3i{radius[0], y, z});
            callback(center + Point3i{-radius[0], y+1, z});
          }
    }
  */
  return num;
}


#endif // MISC_HPP
