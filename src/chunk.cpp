#include "chunk.hpp"

#include <cstring>
#include <algorithm>
#include <QMatrix4x4>

#include "block.hpp"
#include "shader.hpp"

std::vector<cModelObj> cChunk::mModels;

cChunk::cChunk(Vector3i size)
  : mSize(size), mBlocks(nullptr, Vector3i(size/2), size), mMeshes((int)block_t::COUNT)
{
  //const int numBlocks = mSize[0] * mSize[1] * mSize[2];
  //mBlocks = new block_t[numBlocks];
  //memset(mBlocks, (int)block_t::NONE, numBlocks*sizeof(block_t));
}

cChunk::~cChunk()
{
  
}


void cChunk::loadModels()
{
  mModels.emplace_back("");
  mModels.emplace_back("./res/cube.obj");
  mModels.emplace_back("./res/device.obj");
  mModels.emplace_back("./res/cpu.obj");
  mModels.emplace_back("./res/memory.obj");
}

void cChunk::modelInitGL(cShader *shader)
{
  for(auto &m : mModels)
    {
      m.initGL(shader);
    }
}
void cChunk::modelCleanupGL()
{
  for(auto &m : mModels)
    {
      m.cleanupGL();
    }
}

void cChunk::initGL(cShader *shader)
{
  for(auto &m : mMeshes)
    {
      m.initGL(shader);
    }
}

void cChunk::cleanupGL()
{
  for(auto &m : mMeshes)
    {
      m.cleanupGL();
    }
}


void cChunk::render(cShader *shader, Matrix4 pvm)
{
  shader->setUniform("pvm", pvm);
  for(int i = 0; i < mMeshes.size(); i++)
    {
      shader->setUniform("uBlockType", i - 1);
      mMeshes[i].render(shader);
    }
  
}


void cChunk::updateMesh()
{
  std::vector<objl::Vertex> vertices[(int)block_t::COUNT];
  std::vector<unsigned int> indices[(int)block_t::COUNT];

  for(int i = 0; i < (int)block_t::COUNT; i++)
    {
      vertices[i].reserve(24 * mBlocks.size());
      indices[i].reserve(24 * 3 * mBlocks.size());
    }
  
  // TODO: Cull touching block faces.
  
  LOGD("iterating blocks");
  
  mBlocks.iterate([&vertices, &indices, this](Point3i *p, cBlock *b)
		  {
		    //std::cout << "Point: " << *p << ", pb: " << (long)b << ", type: " << (int)b->type << "\n";

		    //
		    const std::vector<objl::Vertex> &v = mModels[(int)b->type].getVertices();
		    const std::vector<unsigned int> &i = mModels[(int)b->type].getIndices();

		    // POSZ --> v[0/1/2] / side 1 (i[0:5])
		    // POSY --> v[2/3/4] / side 2 (i[6:11])
		    // NEGZ --> v[4/5/6] / side 3 (i[12:17])
		    // NEGY --> v[0/6/7] / side 4 (i[18:23])
		    // POSX --> v[1/3/7] / side 5 (i[24:29])
		    // NEGX --> v[0/4/6] / side 6 (i[30:35])
		    //blockSide_t adj = mBlocks.adjacent(*p);

		    //if(adj & BS_ALL != BS_ALL)
		    //  { // not completely covered by other blocks
			
		    unsigned int numVert = vertices[(int)b->type].size();
		    //std::cout << ", numvert: " << numVert << "\n";
		    for(const auto &vv : v)
		      {
			objl::Vertex nv(vv);
			
			nv.Position.X += (*p)[0];
			nv.Position.Y += (*p)[1];
			nv.Position.Z += (*p)[2];
			
			vertices[(int)b->type].push_back(nv);
		      }
		    //std::cout << ", indices...\n";
		    for(auto ii : i)
		      {
			indices[(int)b->type].push_back((int)ii + numVert);
		      }
		    // }
		    
		    return true;
		  } );

  /*
  for(int t = 0; t < (int)block_t::COUNT; t++)
    {
      for(int i = 0; i < vertices[t].size(); i++)
	{
	  if(vertices[i] == vertices[i+1])
	    {
	      
	    }
	}
    }
  */  
  for(int i = 0; i < (int)block_t::COUNT; i++)
    {
      //LOGD("setting mesh %d", i);
      mMeshes[i].setMesh(vertices[i], indices[i]);
    }
}

bool cChunk::placeBlocks(const std::vector<Point3i> &pos, const std::vector<cBlock*> &blocks)
{
  bool success = true;
  for(int i = 0; i < pos.size(); i++)
    {
      if(!mBlocks.contains(pos[i]))
	{
	  std::cout << "Placing block --> " << blocks[i] << "\n";
	  mBlocks.insert(pos[i], blocks[i]);
	  mBlockBB.emplace_back(Vector3f(pos[i]) + 0.5f, Vector3f{1.0f, 1.0f, 1.0f});
	}
      else { success = false; }
    }
  updateMesh();
  return success;
}
bool cChunk::placeBlocks(const std::vector<Point3i> &pos, const std::vector<block_t> &types)
{
  bool success = true;
  for(int i = 0; i < pos.size(); i++)
    {
      if(!mBlocks.contains(pos[i]))
	{
	  mBlocks.insert(pos[i], types[i]);
	  mBlockBB.emplace_back(Vector3f(pos[i]) + 0.5f, Vector3f{1.0f, 1.0f, 1.0f});
	}
      else { success = false; }
    }
  updateMesh();
  return success;
}
block_t cChunk::pickBlock(const Point3i &pos)
{
  mBlocks.remove(pos, true);
  updateMesh();
  return block_t::NONE;
}

std::vector<cBoundingBox> cChunk::collides(const cBoundingBox &box, bool edge)
{
  cBoundingBox bbox(Point3f(), Vector3f{1.0f, 1.0f, 1.0f});

  std::vector<Point3i*> points;
  std::vector<cBlock*> nearby;
  mBlocks.getPointsInBox(box.minPoint() - Vector3f{3, 3, 3}, box.maxPoint() + Vector3f{3, 3, 3}, points, nearby);

  std::vector<cBoundingBox> collide;
  collide.reserve(nearby.size());
  for(int i = 0; i < nearby.size(); i++)
    {
      bbox.setCenter(Point3f(*points[i]) + Vector3f{0.5f, 0.5f, 0.5f});
      if(nearby[i]->type != block_t::NONE && (edge ? box.collidesEdge(bbox) : box.collides(bbox)))
	{
	  collide.push_back(bbox);
	}
    }
  
  return collide;
}

std::vector<Vector3f> cChunk::correction(const cBoundingBox &box, const std::vector<cBoundingBox> &blocks, const Vector3f &dPos, const Vector3i &onGround)
{
  std::vector<Vector3f> corrections;
  corrections.reserve(blocks.size());
  for(auto &b : blocks)
    {
      corrections.push_back(box.correction(b, dPos, onGround));
    }
  return corrections;
}



bool cChunk::closestIntersection(const Point3f &pos, const Vector3f &dir, float radius, cBlock *&blockOut, Point3i &posOut, Vector3i &faceOut)
{
  return mBlocks.getClosestBlock(pos, dir, radius, blockOut, posOut, faceOut);
}
