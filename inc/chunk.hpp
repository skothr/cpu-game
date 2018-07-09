#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <vector>
#include "geometry.hpp"
#include "block.hpp"
#include "vector.hpp"
#include "model.hpp"
#include "octree.hpp"
#include "mesh.hpp"
#include "collision.hpp"

class cShader;





















template<int SX, int SY, int SZ>
class cChunkData
{
public:
  cChunkData(const Point3i &chunkPos)
    : mPos(chunkPos)
  {
    
  }

  bool placeBlock(const Point3i &pos, block_t type)
  {
    const int index = getIndex(pos - mPos);
    if(data[index].type == block_t::NONE)
      {
	data[index].type = type;
	mNumBlocks++;
	return true;
      }
    else
      {
	std::cout << "Block spot occupied! --> " << pos << " current: " << (int)data[index].type << "\n";
	return false;
      }
  }

  int size() const
  {
    return mNumBlocks;
  }

  block_t pickBlock(const Point3i &pos)
  {
    const int index = getIndex(pos - mPos);
    block_t type = data[index].type;
    data[index].type = block_t::NONE;
    mNumBlocks--;
    return type;
  }

  bool contains(const Point3i &pos)
  {
    if(pos[0] - mPos[0] >= 0 && pos[0] - mPos[0] < SX &&
       pos[1] - mPos[1] >= 0 && pos[1] - mPos[1] < SY &&
       pos[2] - mPos[2] >= 0 && pos[2] - mPos[2] < SZ )
      {
	return data[getIndex(pos - mPos)].type != block_t::NONE;
      }
    else
      { return false; }
  }

  cBlock* get(const Point3i &pos)
  {
    if(pos[0] - mPos[0] >= 0 && pos[0] - mPos[0] < SX &&
       pos[1] - mPos[1] >= 0 && pos[1] - mPos[1] < SY &&
       pos[2] - mPos[2] >= 0 && pos[2] - mPos[2] < SZ )
      {
	return &data[getIndex(pos - mPos)];
      }
    else
      { return nullptr; }
  }
  
  std::vector<Point3i> getPointsInBox(const Point3f &min, const Point3f &max)
  {
    Point3i minP{std::floor(min[0] - mPos[0]), std::floor(min[1] - mPos[1]), std::floor(min[2] - mPos[2])};
    Point3i maxP{std::ceil(max[0] - mPos[0]), std::ceil(max[1] - mPos[1]), std::ceil(max[2] - mPos[2])};

    if(minP[0] < 0)
      { minP[0] = 0; }
    if(minP[1] < 0)
      { minP[1] = 0; }
    if(minP[2] < 0)
      { minP[2] = 0; }
    if(maxP[0] >= SX)
      { maxP[0] = SX - 1; }
    if(maxP[1] >= SY)
      { maxP[1] = SY - 1; }
    if(maxP[2] >= SZ)
      { maxP[2] = SZ - 1; }

    std::vector<Point3i> points;
    points.reserve((maxP[0] - minP[0])*(maxP[1] - minP[1])*(maxP[2] - minP[2]));
    int index = getIndex(minP);
    for(int z = minP[2]; z < maxP[2]; z++)
      {
	for(int y = minP[1]; y < maxP[1]; y++)
	  {
	    for(int x = minP[0]; x < maxP[0]; x++)
	      {
		index = getIndex(Point3i{x,y,z});
		if(data[index].type != block_t::NONE)
		  {
		    points.emplace_back(Point3i{x,y,z});
		  }
	      }
	  }
      }
    return points;
  }

  
  float mod(float value, float modulus)
  {
    return std::fmod(value, modulus);
  }

  float intbound(float s, float ds)
  {
    return (ds > 0 ? (std::floor(s) + 1 - s) / std::abs(ds) : (ds < 0 ? (s - std::floor(s)) / std::abs(ds) : 0));
  }

  // ray-world intersection
  bool getClosestBlock(const Point3f &p, const Vector3f &d, float radius, cBlock *&blockOut, Point3i &posOut, Vector3i &faceOut)
  {
    if(d[0] == 0 && d[1]==0 && d[2]==0)
      { return false; }
  
    Vector3f step{  (d[0] > 0 ? 1 : (d[0] < 0 ? -1 : 1)),
		    (d[1] > 0 ? 1 : (d[1] < 0 ? -1 : 1)),
		    (d[2] > 0 ? 1 : (d[2] < 0 ? -1 : 1)) };
    Vector3f tMax{intbound(p[0], d[0]), intbound(p[1], d[1]), intbound(p[2], d[2])};
    Vector3f delta{d[0] == 0 ? 0 : (step[0] / d[0]),
		   d[1] == 0 ? 0 : (step[1] / d[1]),
		   d[2] == 0 ? 0 : (step[2] / d[2]) };
    radius /= d.length();

    Point3f pi{std::floor(p[0]), std::floor(p[1]), std::floor(p[2])};
  
    while((step[0] > 0 ? (pi[0] < SX) : (pi[0] >= 0)) &&
	  (step[1] > 0 ? (pi[1] < SY) : (pi[1] >= 0)) &&
	  (step[2] > 0 ? (pi[2] < SZ) : (pi[2] >= 0)) )
      {
	if(!(pi[0] < 0 || pi[1] < 0 || pi[2] < 0 || pi[0] >= SX || pi[1] >= SY || pi[2] >= SZ))
	  {
	    cBlock *b = get(pi);
	    if(b && b->type != block_t::NONE)
	      {
		blockOut = b;
		posOut = pi;
		return true;
	      }
	  }
	
	if(tMax[0] < tMax[1])
	  {
	    if(tMax[0] < tMax[2])
	      {
		if(tMax[0] > radius)
		  { break; }
		pi[0] += step[0];
		tMax[0] += delta[0];
		faceOut = Vector3i{-step[0], 0, 0};
	      }
	    else
	      {
		if(tMax[2] > radius)
		  { break; }
		pi[2] += step[2];
		tMax[2] += delta[2];
		faceOut = Vector3i{0, 0, -step[2]};
	      }
	  }
	else
	  {
	    if(tMax[1] < tMax[2])
	      {
		if(tMax[1] > radius)
		  { break; }
		pi[1] += step[1];
		tMax[1] += delta[1];
		faceOut = Vector3i{0, -step[1], 0};
	      }
	    else
	      {
		if(tMax[2] > radius)
		  { break; }
		pi[2] += step[2];
		tMax[2] += delta[2];
		faceOut = Vector3i{0, 0, -step[2]};
	      }
	  }
      }
    return false;
  }
      
  cBlock data[SX * SY * SZ];

private:
  Point3i mPos;
  int mNumBlocks = 0;
  int getIndex(const Point3i &p) const
  {
    return p[2]*SX*SY + p[1]*SX + p[0];
  }
};

#define CHUNK_SIZE 16

class cChunk
{
public:
  cChunk(Vector3i size);
  ~cChunk();

  static void loadModels();
  static void modelInitGL(cShader *shader);
  static void modelCleanupGL();
  void initGL(cShader *shader);
  void cleanupGL();
  
  void render(cShader *shader, Matrix4 pvm);
  
  bool placeBlocks(const std::vector<Point3i> &pos, const std::vector<cBlock*> &blocks);
  bool placeBlocks(const std::vector<Point3i> &pos, const std::vector<block_t> &types);
  block_t pickBlock(const Point3i &pos);
  //bool closestIntersection(Point3f pos, Vector3f dir, Point3i &blockOut, Vector3i &faceOut)
  bool closestIntersection(const Point3f &pos, const Vector3f &dir, float radius, cBlock *&blockOut, Point3i &posOut, Vector3i &faceOut);

  std::vector<cBoundingBox> collides(const cBoundingBox &box, bool edge = false);
  std::vector<Vector3f> correction(const cBoundingBox &box, const std::vector<cBoundingBox> &block, const Vector3f &dPos, const Vector3i &onGround);
  
private:
  static std::vector<cModelObj> mModels;
  std::vector<cMesh> mMeshes;
  Vector3i mSize;
  std::vector<cBoundingBox> mBlockBB;

  cChunkData<CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE> mData;
  
  void updateMesh();
};


#endif // CHUNK_HPP
