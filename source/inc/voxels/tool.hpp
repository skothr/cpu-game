#ifndef TOOL_HPP
#define TOOL_HPP

#include "vector.hpp"

enum class tool_t
  {
   NONE = 0,
   SPHERE,
   CUBE,
   LINE
  };


struct ToolParams
{
  struct Sphere
  {
    int radius;
  };
  struct Cube
  {
    Point3i p1;
    Point3i p2;
  };
  struct Line
  {
    int dim;
    int length;
  };
  
  Sphere sphere;
  Cube cube;
  Line line;
};

// SPHERE: center, radius
// CUBE: p1, p2 
// LINE: p, dim, length
//


#endif // TOOL_HPP
