#include "chunk.hpp"

#include <iostream>
#include "nmmintrin.h"

inline int sumBits(uint32_t i)
{
  i = i - ((i >> 1) & 0x55555555);
  i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
  return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

const Point3i cChunk::size { sizeX, sizeY, sizeZ };

// class definitions
cChunk::cChunk()
  : cChunk(Point3i())
{ }

cChunk::cChunk(const Point3i &worldPos)
  : mWorldPos(worldPos)
{
  mMesh = new cChunkMesh();
}

cChunk::cChunk(const cChunk &other)
  : mWorldPos(other.mWorldPos), mNeighbors(other.mNeighbors),
    mData(other.mData), mMesh(other.mMesh)
{ }

cChunk::~cChunk()
{ delete mMesh; }

void cChunk::setWorldPos(const Point3i &newPos)
{ mWorldPos = newPos; }
Point3i cChunk::pos() const
{ return mWorldPos; }

bool cChunk::empty() const
{
  for(const auto &block : mData.mData)
    { if(block.active()) return false; }
  return true;
}

cBlock* cChunk::at(int bx, int by, int bz)
{ return at(Point3i{bx, by, bz}); }
cBlock* cChunk::at(Point3i bp)
{
  blockSide_t side = blockSide_t::NONE;
  if(bp[0] < 0)
    {
      bp[0] += size[0];
      side |= blockSide_t::NX;
    }
  else if(bp[0] >= size[0])
    {
      bp[0] -= size[0];
      side |= blockSide_t::PX;
    }
  if(bp[1] < 0)
    {
      bp[1] += size[1];
      side |= blockSide_t::NY;
    }
  else if(bp[1] >= size[1])
    {
      bp[1] -= size[1];
      side |= blockSide_t::PY;
    }
  if(bp[2] < 0)
    {
      bp[2] += size[2];
      side |= blockSide_t::NZ;
    }
  else if(bp[2] >= size[2])
    {
      bp[2] -= size[2];
      side |= blockSide_t::PZ;
    }
  
  const int bi = cChunkData::index(bp[0], bp[1], bp[2]);

  if(side == blockSide_t::NONE)
    { return &mData.mData[bi]; }
  else
    {
      cChunk *n = mNeighbors[side];
      if(n)
        { return &n->mData.mData[bi]; }
      else
        {
          LOGD("NEIGHBOR NULL! SIDE: %X", (int)side);
          return nullptr;
        }
    }
}

block_t cChunk::get(int bx, int by, int bz) const
{ return mData.get(bx, by, bz); }
block_t cChunk::get(const Point3i &bp) const
{ return mData.get(bp[0], bp[1], bp[2]); }

void cChunk::setNeighbor(blockSide_t side, cChunk *chunk)
{ mNeighbors[side] = chunk; }
void cChunk::unsetNeighbor(blockSide_t side)
{
  if(mNeighbors[side])
    { mNeighbors[side]->mNeighbors[oppositeSide(side)] = nullptr; }
  mNeighbors[side] = nullptr;
}
void cChunk::unsetNeighbors()
{
  for(auto &n : mNeighbors)
    {
      unsetNeighbor(n.first);
    }
}

cChunk* cChunk::getNeighbor(blockSide_t side)
{
  return mNeighbors[side];
}

bool cChunk::set(int bx, int by, int bz, block_t type)
{
  if(type != mData.get(bx, by, bz))
    {
      blockSide_t side = mData.set(bx, by, bz, type);
      if(side != blockSide_t::NONE)
        {
          cBlock *b = at(bx, by, bz);
          Point3i p{bx, by, bz};
          Point3i np({(bool)(side & blockSide_t::PX) ? 0 :
                      ((bool)(side & blockSide_t::NX) ? sizeX-1 : bx),
                      (bool)(side & blockSide_t::PY) ? 0 :
                      ((bool)(side & blockSide_t::NY) ? sizeY-1 : by),
                      (bool)(side & blockSide_t::PZ) ? 0 :
                      ((bool)(side & blockSide_t::NZ) ? sizeZ-1 : bz)});
          
          for(int i = 0; i < 3; i++)
            {
              blockSide_t dSide = (i==0 ? (side & (blockSide_t::PX | blockSide_t::NX)) :
                                   (i==1 ? (side & (blockSide_t::PY | blockSide_t::NY)) :
                                    (side & (blockSide_t::PZ | blockSide_t::NZ))));
              
              if(dSide != blockSide_t::NONE)
                {
                  cChunk *n = mNeighbors[dSide];
                  if(n)
                    {
                      Point3i pp = p;
                      pp[i] = np[i];
                      b->setActive(dSide, n->mData.get(pp[0], pp[1], pp[2]));
                      n->mData.at(pp[0], pp[1], pp[2])->setActive(oppositeSide(dSide), b->type);
                      n->mData.updateLighting(pp[0], pp[1], pp[2]);
                      n->mMeshDirty = true;
                      n->mMeshUploaded = false;
                    }
                }
            }
          
        }
      mData.updateLighting(bx, by, bz);
      mDirty = true;
      mMeshDirty = true;
      mMeshUploaded = false;
      return true;
    }
  else
    { return false; }
}
bool cChunk::set(const Point3i &bp, block_t type)
{
  return set(bp[0], bp[1], bp[2], type);
}

void cChunk::initGL(cShader *shader)
{
  if(!mMesh->initGL(shader, &mVert, &mInd))
    { uploadMesh(); }
}
void cChunk::cleanupGL()
{
  mMesh->cleanupGL();
}
void cChunk::render()
{
  mMesh->render();
}




bool cChunk::isLoaded() const
{ return mLoaded; }
void cChunk::setLoaded(bool loaded)
{ mLoaded = loaded; }
bool cChunk::isDirty() const
{ return mDirty; }
void cChunk::setDirty(bool dirty)
{ mDirty = dirty; }
bool cChunk::meshDirty() const
{ return mMeshDirty; }
void cChunk::setMeshDirty(bool dirty)
{ mMeshDirty = dirty; }
bool cChunk::meshUploaded() const
{ return mMeshUploaded; }
void cChunk::setMeshUploaded(bool uploaded)
{ mMeshUploaded = uploaded; }


void cChunk::updateBlocks()
{
  /*
  for(int by = 0; by < sizeY; by++)
    for(int bz = 0; bz < sizeZ; bz++)
      for(int bx = 0; bx < sizeX; bx++)
        {
          blockSide_t edges = mData.chunkEdge(bx, by, bz);
          cBlock *b = mData.at(bx, by, bz);
          cChunk *n = mNeighbors[edges];
          if(n)
            {
              const Point3i np({(bool)(edges & blockSide_t::PX) ? sizeX-1 :
                                ((bool)(edges & blockSide_t::NX) ? 0 : bx),
                                (bool)(edges & blockSide_t::PY) ? sizeY-1 :
                                ((bool)(edges & blockSide_t::NY) ? 0 : by),
                                (bool)(edges & blockSide_t::PZ) ? sizeZ-1 :
                                ((bool)(edges & blockSide_t::NZ) ? 0 : bz) });
              if(sumBits((int)edges) == 1)
                {
                  b->setActive(edges, n->mData.get(np[0], np[1], np[2]));
                  n->mData.at(np[0], np[1], np[2])->setActive(oppositeSide(edges), b->type);
                }
              n->mData.updateLighting(np[0], np[1], np[2]);
            }
          mData.updateLighting(bx, by, bz);
        }
  */
  mData.updateBlocks();
}

// TODO: Run length encoding
int cChunk::serialize(uint8_t *dataOut) const
{
  int size = 0;
  for(const auto &block : mData.mData)
    {
      block.serialize(dataOut + size);
      size += cBlock::dataSize;
    }
  return size;
}
void cChunk::deserialize(const uint8_t *dataIn, int bytes)
{
  int offset = 0;
  for(auto &block : mData.mData)
    {
      block.deserialize((dataIn + offset), cBlock::dataSize);
      offset += cBlock::dataSize;
      if(offset + cBlock::dataSize >= bytes)
        { break; }
    }
  
  for(int by = 0; by < sizeY; by++)
    for(int bz = 0; bz < sizeZ; bz++)
      for(int bx = 0; bx < sizeX; bx++)
        { updateLighting(bx, by, bz); }
  
  // update active sides
  updateBlocks();
}

std::string cChunk::toString() const
{
  return "CHUNK (TODO: cChunk.toString())";
}

std::ostream& operator<<(std::ostream &os, const cChunk &chunk)
{
  os << "CHUNK (TODO: cChunk::operator<<)";
  return os;
}

#define NEIGHBOR_TEX false

void cChunk::updateLighting(int bx, int by, int bz)
{
  mData.updateLighting(bx, by, bz);
}


inline blockSide_t dimSide(int dim, int sign)
{
  int side = dim + (sign < 0 ? 3 : 0);
  return (blockSide_t)(1 << side);
}

inline int getAO(int e1, int e2, int c)
{
  if(e1 == 0 && e2 == 0)
    { return 0; }
  else
    { return (e1 + e2 + c); }
}

uint8_t cChunk::getLighting(const Point3i &bp, const Point3i &vp, blockSide_t side)
{
  blockSide_t nSides = mData.chunkEdge(bp[0], bp[1], bp[2]);
  
  if(nSides != blockSide_t::NONE)
    {
      const int dim = sideDim(side);
      const int dim1 = (dim+1) % 3;
      const int dim2 = (dim+2) % 3;

      const int e1 = bp[dim1] + vp[dim1] * 2 - 1;
      const int e2 = bp[dim2] + vp[dim2] * 2 - 1;
      
      Point3i bi;
      bi[dim] = bp[dim] + sideSign(side);
      bi[dim1] = e1;
      bi[dim2] = e2;

      int lc=1;
      int le1=1;
      int le2=1;
      
      cBlock *adj = at(bi);
      if(adj)
        { lc = adj->lightLevel; }
      
      bi[dim1] = bp[dim1];
      adj = at(bi);
      if(adj)
        { le1 = adj->lightLevel; }
      
      bi[dim1] = e1;
      bi[dim2] = bp[dim2];
      adj = at(bi);
      if(adj)
        { le2 = adj->lightLevel; }
      
      return getAO(le1, le2, lc);
    }
  else
    {
      //LOGD("Got lighting (inside).");
      return mData.getLighting(bp[0], bp[1], bp[2], vp[0], vp[1], vp[2], side);
    }
}

static std::unordered_map<blockSide_t, std::array<cSimpleVertex, 4>> faceVertices
  {
    {blockSide_t::PX,
	{cSimpleVertex(Point3f{1, 0, 0}, Vector3f{1, 0, 0}, Vector2f{0.0f, 0.0f} ),
	   cSimpleVertex(Point3f{1, 1, 1}, Vector3f{1, 0, 0}, Vector2f{1.0f, 1.0f} ),
	   cSimpleVertex(Point3f{1, 0, 1}, Vector3f{1, 0, 0}, Vector2f{0.0f, 1.0f} ),
	   cSimpleVertex(Point3f{1, 1, 0}, Vector3f{1, 0, 0}, Vector2f{1.0f, 0.0f} ) } },
      {blockSide_t::PY,
	  {cSimpleVertex(Point3f{0, 1, 0}, Vector3f{0, 1, 0}, Vector2f{0.0f, 0.0f} ),
	     cSimpleVertex(Point3f{1, 1, 1}, Vector3f{0, 1, 0}, Vector2f{1.0f, 1.0f} ),
	     cSimpleVertex(Point3f{1, 1, 0}, Vector3f{0, 1, 0}, Vector2f{0.0f, 1.0f} ),
	     cSimpleVertex(Point3f{0, 1, 1}, Vector3f{0, 1, 0}, Vector2f{1.0f, 0.0f} ) } },
	{blockSide_t::PZ,
	    {cSimpleVertex(Point3f{0, 0, 1}, Vector3f{0, 0, 1}, Vector2f{0.0f, 0.0f} ),
	       cSimpleVertex(Point3f{1, 1, 1}, Vector3f{0, 0, 1}, Vector2f{1.0f, 1.0f} ),
	       cSimpleVertex(Point3f{0, 1, 1}, Vector3f{0, 0, 1}, Vector2f{0.0f, 1.0f} ),
	       cSimpleVertex(Point3f{1, 0, 1}, Vector3f{0, 0, 1}, Vector2f{1.0f, 0.0f} ) } },
	  {blockSide_t::NX,
	      {cSimpleVertex(Point3f{0, 0, 0}, Vector3f{-1, 0, 0}, Vector2f{0.0f, 0.0f} ),
		 cSimpleVertex(Point3f{0, 1, 1}, Vector3f{-1, 0, 0}, Vector2f{1.0f, 1.0f} ),
		 cSimpleVertex(Point3f{0, 1, 0}, Vector3f{-1, 0, 0}, Vector2f{0.0f, 1.0f} ),
		 cSimpleVertex(Point3f{0, 0, 1}, Vector3f{-1, 0, 0}, Vector2f{1.0f, 0.0f} ) } },
	    {blockSide_t::NY,
		{cSimpleVertex(Point3f{0, 0, 0}, Vector3f{0, -1, 0}, Vector2f{0.0f, 0.0f} ),
		   cSimpleVertex(Point3f{1, 0, 1}, Vector3f{0, -1, 0}, Vector2f{1.0f, 1.0f} ),
		   cSimpleVertex(Point3f{0, 0, 1}, Vector3f{0, -1, 0}, Vector2f{0.0f, 1.0f} ),
		   cSimpleVertex(Point3f{1, 0, 0}, Vector3f{0, -1, 0}, Vector2f{1.0f, 0.0f} ) } },
	      {blockSide_t::NZ,
		  {cSimpleVertex(Point3f{0, 0, 0}, Vector3f{0, 0, -1}, Vector2f{0.0f, 0.0f} ),
		     cSimpleVertex(Point3f{1, 1, 0}, Vector3f{0, 0, -1}, Vector2f{1.0f, 1.0f} ),
		     cSimpleVertex(Point3f{1, 0, 0}, Vector3f{0, 0, -1}, Vector2f{0.0f, 1.0f} ),
		     cSimpleVertex(Point3f{0, 1, 0}, Vector3f{0, 0, -1}, Vector2f{1.0f, 0.0f} ) } } };


static std::array<unsigned int, 6> faceIndices = { 0, 1, 2, 3, 1, 0 };
static std::array<unsigned int, 6> flippedIndices = { 0, 3, 2, 1, 2, 3 };

void cChunk::updateMesh()
{
  // recompile mesh
  mVert.clear();
  mInd.clear();
  
  const Point3i chunkOffset = (mWorldPos *
			       Point3i{sizeX, sizeY, sizeZ} );
  
  //LOGD("CHUNK UPDATING MESH");
  // iterate over the chunk's blocks and compile all face vertices.
  for(int bx = 0; bx < sizeX; bx++)
    for(int by = 0; by < sizeY; by++)
      for(int bz = 0; bz < sizeZ; bz++)
	{
          //LOGD("BLOCK: %d, %d, %d", bx, by, bz);
	  const cBlock *block = mData.at(bx, by, bz);
	  if(block && block->active() && isSimpleBlock(block->type))
	    {
              //LOGD("ACTIVE");
              const Point3i bp{bx, by, bz};
              const Point3i vOffset = chunkOffset + bp;
              
	      for(auto &f : faceVertices)
		{ // check if each face direction is active
		  if((bool)(block->activeSides & f.first))
		    {
		      const unsigned int numVert = mVert.size();

                      int vn = 0;
                      int sum0 = 0;
                      int sum1 = 0;
		      for(auto &v : f.second)
			{
                          const int lighting = getLighting(bp, v.pos, f.first);
                          //LOGD("VERTEX -> %f, %f, %f", v.pos[0], v.pos[1], v.pos[2]);
                          mVert.emplace_back(v.pos + vOffset,
                                             v.normal,
                                             v.texcoord,
                                             (int)block->type - 1,
                                             lighting / 3.0f );
                          if(vn < 2)
                            { sum0 += lighting; }
                          else
                            { sum1 += lighting; }
                          vn++;
                        }
                      if(sum1 >= sum0)
                        {
                          for(auto i : flippedIndices)
                            { mInd.push_back(numVert + i); }
                        }
                      else
                        {
                          for(auto i : faceIndices)
                            { mInd.push_back(numVert + i); }
                        }
                    }		      
                }
            }
          // TODO: Complex blocks
        }
  //LOGD("CHUNK MESH UPDATE FINISHED --> %d", mInd.size());
}

void cChunk::uploadMesh()
{
  mMesh->finishUpdating(mInd.size(), mVert, mInd);
}
