#include "chunkMap.hpp"



cSubChunk::cSubChunk()
{

}

cBlock* cSubChunk::operator()(int x, int y, int z)
{
  return &mData[getIndex(x, y, z)];
}
const cBlock* cSubChunk::operator()(int x, int y, int z) const
{
  return &mData[getIndex(x, y, z)];
}
cBlock* cSubChunk::operator()(const Point3i &p)
{
  return &mData[getIndex(p[0], p[1], p[2])];
}
const cBlock* cSubChunk::operator()(const Point3i &p) const
{
  return &mData[getIndex(p[0], p[1], p[2])];
}

cBlock* cSubChunk::get(const Point3i &p)
{
  return &mData[getIndex(p[0], p[1], p[2])];
}
const cBlock* cSubChunk::get(const Point3i &p) const
{
  return &mData[getIndex(p[0], p[1], p[2])];
}


cVirtualChunk::cVirtualChunk(const Point2i &offset)
  : mWorldOffset(offset)
{

}
  
cSubChunk* cVirtualChunk::operator()(int x, int y, int z)
{
  return &mData[getIndex(x, y, z)];
}
const cSubChunk* cVirtualChunk::operator()(int x, int y, int z) const
{
  return &mData[getIndex(x, y, z)];
}
cSubChunk* cVirtualChunk::operator()(const Point3i &p)
{
  return &mData[getIndex(p[0], p[1], p[2])];
}
const cSubChunk* cVirtualChunk::operator()(const Point3i &p) const
{
  return &mData[getIndex(p[0], p[1], p[2])];
}
  
cBlock* cVirtualChunk::get(const Point3i &p)
{
  return mData[getIndex(p[0] / CHUNK_SIZE,
			p[1] / CHUNK_SIZE,
			p[2] / CHUNK_SIZE )].get(Point3i{p[0] % CHUNK_SIZE,
							 p[1] % CHUNK_SIZE,
							 p[2] % CHUNK_SIZE} );
}
const cBlock* cVirtualChunk::get(const Point3i &p) const
{
  return get(p);
}


std::vector<cModelObj> cChunkMap::mModels;

cChunkMap::cChunkMap(const Point2i &offset)
  : mWorldOffset(offset), mShape{0,0},
    mMeshes((int)block_t::COUNT)
{ }
    
cChunkMap::~cChunkMap()
{
  for(auto &v : mData)
    {
      for(auto c : v)
	{ delete c; }
    }
}

cBlock* cChunkMap::get(const Point3i &worldPos)
{
  Point2i relative = (Point2i(worldPos) - mWorldOffset);
  Point2i chunkIndex = relative / CHUNK_BLOCKS;
  Point3i chunkPos{relative[0] % CHUNK_BLOCKS,
		   relative[1] % CHUNK_BLOCKS,
		   worldPos[2] };
  return mData[chunkIndex[0]][chunkIndex[1]]->get(chunkPos);
}
const cBlock* cChunkMap::get(const Point3i &worldPos) const
{
  return get(worldPos);
}

bool cChunkMap::set(const Point3i &p, block_t type)
{
  cBlock *b = get(p);
  if(b && b->type == block_t::NONE || type == block_t::NONE)
    {
      b->type = type;
      if(initialized)
	updateMesh();
      return true;
    }
  else
    {
      return false;
    }
}

void cChunkMap::loadChunk(const Point2i &chunkIndex)
{
  LOGD("Loading chunk...");
  if(chunkIndex[0] >= mShape[0])
    {
      LOGD("Resize x...");
      mData.resize(chunkIndex[0] + 1);
      mShape[0] = chunkIndex[0] + 1;
    }
  if(chunkIndex[1] >= mShape[1])
    {
      LOGD("Resize y...");
      for(int i = 0; i < mData.size(); i++)
	{ mData[i].resize(chunkIndex[1] + 1, nullptr); }
      mShape[1] = chunkIndex[1] + 1;
    }
  LOGD("Get chunk...");
  cVirtualChunk* &chunk = mData[chunkIndex[0]][chunkIndex[1]];
  if(!chunk)
    {
      LOGD("Allocate chunk...");
      chunk = new cVirtualChunk(Point2i{chunkIndex[0] * CHUNK_BLOCKS,
					chunkIndex[1] * CHUNK_BLOCKS });
    }
  if(initialized)
    {
      LOGD("Update mesh...");
      updateMesh();
    }
  LOGD("Done loading chunk.");
}


static float mod(float value, float modulus)
{
  return std::fmod(value, modulus);
}

static float intbound(float s, float ds)
{
  return (ds > 0 ? (std::floor(s) + 1 - s) / std::abs(ds) : (ds < 0 ? (s - std::floor(s)) / std::abs(ds) : 0));
}

// ray-world intersection
bool cChunkMap::rayCast(const Point3f &p, const Vector3f &d, float radius,
			cBlock *&blockOut, Point3i &posOut, Vector3i &faceOut )
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
  
  while((step[0] > 0 ? (pi[0] < CHUNK_BLOCKS) : (pi[0] >= 0)) &&
	(step[1] > 0 ? (pi[1] < CHUNK_BLOCKS) : (pi[1] >= 0)) &&
	(step[2] > 0 ? (pi[2] < CHUNK_BLOCKS) : (pi[2] >= 0)) )
    {
      if(!(pi[0] < 0 || pi[1] < 0 || pi[2] < 0 ||
	   pi[0] >= CHUNK_BLOCKS || pi[1] >= CHUNK_BLOCKS || pi[2] >= CHUNK_BLOCKS ))
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


void cChunkMap::loadModels()
{
  mModels.emplace_back("");
  mModels.emplace_back("./res/cube.obj");
  mModels.emplace_back("./res/device.obj");
  mModels.emplace_back("./res/cpu.obj");
  mModels.emplace_back("./res/memory.obj");
  mModels.emplace_back("./res/cube.obj");
}

void cChunkMap::modelInitGL(cShader *shader)
{
  for(auto &m : mModels)
    { m.initGL(shader); }
}
void cChunkMap::modelCleanupGL()
{
  for(auto &m : mModels)
    { m.cleanupGL(); }
}

void cChunkMap::initGL(cShader *shader)
{
  for(auto &m : mMeshes)
    { m.initGL(shader); }
  initialized = true;
  updateMesh();
}

void cChunkMap::cleanupGL()
{
  for(auto &m : mMeshes)
    { m.cleanupGL(); }
  initialized = false;
}


void cChunkMap::render(cShader *shader, Matrix4 pvm)
{
  shader->setUniform("pvm", pvm);
  for(int i = 0; i < mMeshes.size(); i++)
    {
      shader->setUniform("uBlockType", i - 1);
      mMeshes[i].render(shader);
    } 
}


void cChunkMap::updateMesh()
{
  LOGD("Updating mesh...");
  std::vector<objl::Vertex> *vertices[(int)block_t::COUNT];
  std::vector<unsigned int> *indices[(int)block_t::COUNT];
  
  LOGD("Getting vertex/index vectors...");
  for(int i = 0; i < (int)block_t::COUNT; i++)
    {
      LOGD("block_t --> %d...", i);
      vertices[i] = &mMeshes[i].getVertices();
      indices[i] = &mMeshes[i].getIndices();
      vertices[i]->clear();
      indices[i]->clear();
    }
  
  // TODO: Cull touching block faces.
  
  LOGD("iterating blocks");
  for(int z = 0; z < CHUNK_SIZE; z++)
    {
      for(int y = 0; y < CHUNK_SIZE; y++)
	{
	  for(int x = 0; x < CHUNK_SIZE; x++)
	    {
	      cBlock *b = get(Point3i{x, y, z});
	      if(b && b->type != block_t::NONE)
		{
		  std::cout << "block type: " << (int)b->type << "\n";
		  const std::vector<objl::Vertex> &v = mModels[(int)b->type].getVertices();
		  const std::vector<unsigned int> &ind = mModels[(int)b->type].getIndices();

		  std::cout << "vsize: " << v.size() << ", isize: " << ind.size() << "\n"; 

		  unsigned int numVert = vertices[(int)b->type]->size();
		  int skipped = 0;
		  for(int i = 0; i < v.size(); i++)
		    {
		      objl::Vertex nv(v[i]);
		      
		      //if(b->type != block_t::FLOOR ||
		      // !get(Point3i{x + nv.Normal.X, y + nv.Normal.Y, z + nv.Normal.Z}))
			{
			  nv.Position.X += x;
			  nv.Position.Y += y;
			  nv.Position.Z += z;
			  vertices[(int)b->type]->push_back(nv);
			  //indices[(int)b->type]->push_back((int)ind[i] + numVert - skipped);
			}
		      //else
		      //{ skipped++; }
		    }
		  for(int i = 0; i < ind.size(); i++)
		  { indices[(int)b->type]->push_back((int)ind[i] + numVert); }
		}
	    }
	}
    }

  //std::cout << vertices[1]->size() << "\n";
  
  for(auto &m : mMeshes)
    { m.setUpdate(); }
}
