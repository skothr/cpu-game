#ifndef VERTEX_HPP
#define VERTEX_HPP


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

typedef cTexVertex cVertex;

#endif // VETEX_HPP
