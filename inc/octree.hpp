#ifndef OCTROO_HPP
#define OCTREE_HPP
#include "block.hpp"
#include "vector.hpp"
#include <ostream>
#include <functional>

enum blockSide_t
  {
   BS_INVALID = -1,
   BS_NONE = 0x00,
   BS_POSX = 0x01,
   BS_POSY = 0x02,
   BS_POSZ = 0x04,
   BS_NEGX = 0x08,
   BS_NEGY = 0x10,
   BS_NEGZ = 0x20,
   BS_ALL = 0x2F
  };
static blockSide_t& operator|=(blockSide_t &b1, blockSide_t b2) { return b1 = (blockSide_t)((int)b1 | (int)b2); }
static blockSide_t& operator&=(blockSide_t &b1, blockSide_t b2) { return b1 = (blockSide_t)((int)b1 & (int)b2); }
static blockSide_t operator|(blockSide_t b1, blockSide_t b2) { return (blockSide_t)((int)b1 | (int)b2); }
static blockSide_t operator&(blockSide_t b1, blockSide_t b2) { return (blockSide_t)((int)b1 & (int)b2); }

/*
class cBlockArray
{
public:
  cBlockArray();
private:
};
*/


typedef std::function<bool(Point3i *point, cBlock *block)> OctreeIterFunc;

class cOctree
{
public:
  cOctree(cOctree *parent, const Point3i &origin, const Vector3i &size);
  ~cOctree();

  void insert(const Point3i &p, block_t type);
  void insert(const Point3i &p, cBlock *block);
  void remove(const Point3i &p, bool keep = false);
  bool isLeaf() const;
  int size() const;
  bool iterate(OctreeIterFunc f);

  bool getClosestBlock(const Point3f &p, const Vector3f &d, float radius, cBlock *&blockOut, Point3i &posOut, Vector3i &faceOut);

  void getPointsInBox(const Point3f &min, const Point3f &max, std::vector<Point3i*> &pOut, std::vector<cBlock*> &bOut) const;

  blockSide_t getAdjacent(const Point3i &p, std::vector<cBlock*> &blocksOut) const;
  blockSide_t adjacent(const Point3i &p) const;

  void getAll(std::vector<Point3i*> &pointsOut, std::vector<cBlock*> &blocksOut) const;

  cBlock* get(const Point3i &p) const;
  
  bool contains(const Point3i &p) const;
  block_t getType(const Point3i &p) const;
  friend std::ostream& operator<<(std::ostream &os, const cOctree &oct);
  
private:
  Point3i mOrigin;
  Vector3i mSize;
  cOctree *mParent;
  cOctree* mChildren[8];
  Point3i *mPos = nullptr;
  cBlock *mData = nullptr;

  void insert(Point3i *p, cBlock *b);
  int getOctant(const Point3i *p) const;
  void print(std::ostream &os, int depth) const;
};

#endif // OCTREE_HPP
