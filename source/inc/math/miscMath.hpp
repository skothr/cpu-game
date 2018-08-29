#ifndef MISC_HPP
#define MISC_HPP

#include "vector.hpp"
#include <functional>
// inline int sumBits(uint32_t i)
// {
//   i = i - ((i >> 1) & 0x55555555);
//   i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
//   return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
// }

typedef std::function<bool(const Point3i &pos)> centerLoopCallback_t;
inline int loopFromCenter(const Point3i &center, const Point3i &maxRadius,
                          const centerLoopCallback_t &callback, bool reverse = false )
{
  int num = 0;
  int maxR = std::max(maxRadius[0], std::max(maxRadius[1], maxRadius[2]));
  Point3i dimR;
  Vector<bool, 3> skip{false, false, false};
  
  Point3i radius;
  int start = (reverse ? maxR : 0);
  int end = (reverse ? 0 : maxR);
  int step = (reverse ? -1 : 1);
  for(int r = start; (reverse ? r >= end : r <= end); r += step)
    {
      radius[0] = std::min(r, maxRadius[0]);
      radius[1] = std::min(r, maxRadius[1]);

      for(int x = -radius[0]; x < radius[0]; x++)
        for(int z = -maxRadius[2]; z <= maxRadius[2]; z++)
          {
            callback(center + Point3i{x+1, radius[1], z});
            callback(center + Point3i{x, -radius[1], z});
          }
      for(int y = -radius[1]; y < radius[1]; y++)
        for(int z = -maxRadius[2]; z <= maxRadius[2]; z++)
          {
            callback(center + Point3i{radius[0], y, z});
            callback(center + Point3i{-radius[0], y+1, z});
          }
    }
      /*
      // z sides
      if(r <= maxRadius[2])
        {
          for(int x = -radius[0]; x <= radius[0]; x++)
            for(int y = -radius[1]; y <= radius[1]; y++)
              {
                num += (callback(center + Point3i{x, y, -radius[2]}) ? 1 : 0);
                num += (callback(center + Point3i{x, y, radius[2]}) ? 1 : 0);
              }
        }
      // x sides
      if(r <= maxRadius[0])
        {
          for(int y = -radius[1]; y <= radius[1]; y++)
            for(int z = -radius[2]+1; z < radius[2]; z++)
              {
                num += (callback(center + Point3i{radius[0], y, z}) ? 1 : 0);
                num += (callback(center + Point3i{-radius[0], y, z}) ? 1 : 0);
              }
        }
      // y sides
      if(r <= maxRadius[1])
        {
          for(int x = -radius[0]+1; x < radius[0]; x++)
            for(int z = -radius[2]+1; z < radius[2]; z++)
              {
                num += (callback(center + Point3i{x, radius[1], z}) ? 1 : 0);
                num += (callback(center + Point3i{x, -radius[1], z}) ? 1 : 0);
              }
        }
    }
      */
  /*
  for(int r = maxR; r >= 0; r--)
    {
      Point3i radius{r,r,r}; 
      // z sides
      if(r <= mLoadRadius[2])
        {
          for(int x = -radius[0]; x <= radius[0]; x++)
            for(int y = -radius[1]; y <= radius[1]; y++)
              {
                Point3i p{x, y, radius[2]};
                if(mDone.count(Hash::hash(p)) == 0)
                  {
                    addChunkFace(data, offset + p);
                    mDone.insert(Hash::hash(p));
                  }
                p[2] = -radius[2];
                if(mDone.count(Hash::hash(p)) == 0)
                  {
                    addChunkFace(data, offset + p);
                    mDone.insert(Hash::hash(p));
                  }
              }
        }
      // x sides
      if(r <= mLoadRadius[0])
        {
          for(int y = -radius[1]; y <= radius[1]; y++)
            for(int z = -radius[2]; z <= radius[2]; z++)
              {
                Point3i p{radius[0], y, z};
                if(mDone.count(Hash::hash(p)) == 0)
                  {
                    addChunkFace(data, offset + p);
                    mDone.insert(Hash::hash(p));
                  }
                p[0] = -radius[0];
                if(mDone.count(Hash::hash(p)) == 0)
                  {
                   addChunkFace(data, offset + p);
                   mDone.insert(Hash::hash(p));
                  }
              }
        }
      // y sides
      if(r <= mLoadRadius[1])
        {
          for(int x = -radius[0]; x <= radius[0]; x++)
            for(int z = -radius[2]; z <= radius[2]; z++)
              {
                Point3i p{x, radius[1], z};
                if(mDone.count(Hash::hash(p)) == 0)
                  {
                    addChunkFace(data, offset + p);
                    mDone.insert(Hash::hash(p));
                  }
                p[1] = -radius[1];
                if(mDone.count(Hash::hash(p)) == 0)
                  {
                    addChunkFace(data, offset + p);
                    mDone.insert(Hash::hash(p));
                  }
              }
        }
    }

  */
  return num;
}


#endif // MISC_HPP
