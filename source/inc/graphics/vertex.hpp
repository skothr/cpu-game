#ifndef VERTEX_HPP
#define VERTEX_HPP

#include "vector.hpp"

struct cVertex
{
  Point3f pos;
};

struct cColorVertex
{
  float pos[3];
  float normal[3];
  float color[2];
};

struct cTexVertex
{
  float pos[3];
  float normal[3];
  float texcoord[2];
};

struct cSimpleVertex
{
  Point3f pos;
  Vector3f normal;
  Vector3f texcoord;
  float occlusion;
 
  cSimpleVertex() { }
  cSimpleVertex(const Point3f &p, const Vector3f &n, const Vector2f &tex, int type=0,
                float occlusion=0.0f)
    : pos(p), normal(n), texcoord{tex[0], tex[1], type}, occlusion(occlusion)
  { }
};

#endif // VETEX_HPP
