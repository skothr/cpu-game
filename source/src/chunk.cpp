#include "chunk.hpp"

#include <iostream>
#include <unordered_set>
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
  for(int i = 0; i < gBlockSides.size(); i++)
    { mNeighbors.emplace(gBlockSides[i], nullptr); }
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

cBlock* cChunk::at(int bx, int by, int bz, cChunk **neighbor)
{ return at(Point3i{bx, by, bz}, neighbor); }
cBlock* cChunk::at(Point3i bp, cChunk **neighbor)
{
  blockSide_t side = blockSide_t::NONE;

  if(bp[0] < 0)
    {
      bp[0] = (bp[0] + sizeX);
      side |= blockSide_t::NX;
    }
  else if(bp[0] >= size[0])
    {
      bp[0] = bp[0] - sizeX;
      side |= blockSide_t::PX;
    }
  if(bp[1] < 0)
    {
      bp[1] = (bp[1] + sizeY);
      side |= blockSide_t::NY;
    }
  else if(bp[1] >= size[1])
    {
      bp[1] = bp[1] - sizeY;
      side |= blockSide_t::PY;
    }
  if(bp[2] < 0)
    {
      bp[2] = (bp[2] + sizeZ);
      side |= blockSide_t::NZ;
    }
  else if(bp[2] >= size[2])
    {
      bp[2] = bp[2] - sizeZ;
      side |= blockSide_t::PZ;
    }
  
  if(side == blockSide_t::NONE)
    { return &mData.mData[cChunkData::index(bp[0], bp[1], bp[2])]; }
  else
    {
      cChunk *n = mNeighbors[side];
      if(neighbor)
        { *neighbor = n; }
      if(n)
        { return n->at(bp[0], bp[1], bp[2]); }
      else
        { return nullptr; }
    }
}

block_t cChunk::get(int bx, int by, int bz) const
{ return mData.get(bx, by, bz); }
block_t cChunk::get(const Point3i &bp) const
{ return mData.get(bp[0], bp[1], bp[2]); }

void cChunk::setNeighborMirror(blockSide_t side, cChunk *chunk)
{
  mNeighbors[side] = chunk;
}
void cChunk::setNeighbor(blockSide_t side, cChunk *chunk)
{
  setNeighborMirror(side, chunk);
  if(chunk)
    { chunk->setNeighborMirror(oppositeSide(side), this); }
}
void cChunk::unsetNeighborMirror(blockSide_t side)
{
  mNeighbors[side] = nullptr;
  mNeighborsLoaded = false;
}
void cChunk::unsetNeighbor(blockSide_t side)
{
  if(mNeighbors[side])
    { mNeighbors[side]->unsetNeighborMirror(oppositeSide(side)); }
  unsetNeighborMirror(side);
}
void cChunk::unsetNeighbors()
{
  for(auto &n : mNeighbors)
    {
      unsetNeighbor(n.first);
    }
  mNeighborsLoaded = false;
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
          cBlock *b = mData.at(bx, by, bz);
          //if(b->type == block_t::NONE)
          Point3i p{bx, by, bz};
          
          for(int i = 0; i < 3; i++)
            {
              blockSide_t dSide = (i==0 ? (side & (blockSide_t::PX | blockSide_t::NX)) :
                                   (i==1 ? (side & (blockSide_t::PY | blockSide_t::NY)) :
                                    (side & (blockSide_t::PZ | blockSide_t::NZ))));
              
              if(dSide != blockSide_t::NONE)
                {
                  cChunk *n = mNeighbors[dSide];
                  cBlock *nb = at(p + sideDirection(dSide));
                  b->setActive(dSide, nb->type);
                  nb->setActive(oppositeSide(dSide), b->type);
                  n->setMeshDirty(true);
                }
            }

          // update diagonal neighbors 
          blockSide_t xSide = (side & (blockSide_t::PX | blockSide_t::NX));
          blockSide_t ySide = (side & (blockSide_t::PY | blockSide_t::NY));
          blockSide_t zSide = (side & (blockSide_t::PZ | blockSide_t::NZ));

          if(xSide != blockSide_t::NONE)
            {
              if(ySide != blockSide_t::NONE)
                {
                  if(mNeighbors[xSide | ySide])
                    { mNeighbors[xSide | ySide]->setMeshDirty(true); }
                  
                  if(zSide != blockSide_t::NONE && mNeighbors[xSide | ySide | zSide])
                    { mNeighbors[xSide | ySide | zSide]->setMeshDirty(true); }
                }
              if(zSide != blockSide_t::NONE && mNeighbors[xSide | zSide])
                { mNeighbors[xSide | zSide]->setMeshDirty(true); }
            }
          if(ySide != blockSide_t::NONE && zSide != blockSide_t::NONE && mNeighbors[ySide | zSide])
            { mNeighbors[ySide | zSide]->setMeshDirty(true); }
        }
      setLight(false);
      setDirty(true);
      setMeshDirty(true);
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

bool cChunk::neighborsLoaded() const
{
  return mNeighborsLoaded;
}
bool cChunk::checkNeighbors()
{
  if(!mNeighborsLoaded)
    {
      bool loaded = true; 
      for(int i = 0; i < gBlockSides.size(); i++)
        {
          if(!mNeighbors[gBlockSides[i]])
            {
              loaded = false;
              break;
            }
        }
      if(loaded)
        { updateBlocks(); }
      mNeighborsLoaded = loaded;
    }  
  return mNeighborsLoaded;
}
bool cChunk::isLoaded() const
{ return mLoaded; }
void cChunk::setLoaded(bool loaded, bool reset)
{
  mLoaded = loaded;
  if(!mLoaded)
    {
      mNeighborsLoaded = false;
      setMeshDirty(true);
      setMeshUploaded(false);
    }
}
bool cChunk::isDirty() const
{ return mDirty; }
void cChunk::setDirty(bool dirty)
{ mDirty = dirty; }
bool cChunk::meshDirty() const
{ return mMeshDirty; }
void cChunk::setMeshDirty(bool dirty)
{
  mMeshDirty = dirty;
  if(dirty)
    { mMeshUploaded = false; }
}
bool cChunk::meshUploaded() const
{ return mMeshUploaded; }
void cChunk::setMeshUploaded(bool uploaded)
{ mMeshUploaded = uploaded; }


void cChunk::updateBlocks()
{
  for(int by = 0; by < sizeY; by++)
    for(int bz = 0; bz < sizeZ; bz++)
      for(int bx = 0; bx < sizeX; bx++)
        {
          blockSide_t edges = mData.chunkEdge(bx, by, bz);

          /*
          if(edges != blockSide_t::NONE)
            {
              cBlock *b = mData.at(bx, by, bz);
              b->updateOcclusion();
              
              for(int i = 0; i < 3; i++)
                {
                  blockSide_t dSide = (i==0 ? (edges & (blockSide_t::PX | blockSide_t::NX)) :
                                       (i==1 ? (edges & (blockSide_t::PY | blockSide_t::NY)) :
                                        (edges & (blockSide_t::PZ | blockSide_t::NZ))));
              
                  if(dSide != blockSide_t::NONE)
                    {
                      cBlock *nb = at(Point3i{bx, by, bz} + sideDirection(dSide));
                      b->setActive(dSide, nb->type);
                      nb->setActive(oppositeSide(dSide), b->type);
                      mNeighbors[dSide]->setMeshDirty(true);
                    }
                }
              
              // update diagonal neighbors 
              blockSide_t xSide = (edges & (blockSide_t::PX | blockSide_t::NX));
              blockSide_t ySide = (edges & (blockSide_t::PY | blockSide_t::NY));
              blockSide_t zSide = (edges & (blockSide_t::PZ | blockSide_t::NZ));

              if(xSide != blockSide_t::NONE)
                {
                  if(ySide != blockSide_t::NONE)
                    {
                      if(mNeighbors[xSide | ySide])
                        { mNeighbors[xSide | ySide]->setMeshDirty(true); }
                  
                      if(zSide != blockSide_t::NONE && mNeighbors[xSide | ySide | zSide])
                        { mNeighbors[xSide | ySide | zSide]->setMeshDirty(true); }
                    }
                  if(zSide != blockSide_t::NONE && mNeighbors[xSide | zSide])
                    { mNeighbors[xSide | zSide]->setMeshDirty(true); }
                }
              if(ySide != blockSide_t::NONE && zSide != blockSide_t::NONE && mNeighbors[ySide | zSide])
                { mNeighbors[ySide | zSide]->setMeshDirty(true); }
            }
          */
        }
  mData.updateBlocks();
  setMeshDirty(true);
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
      if(offset >= bytes)
        { break; }
    }

  for(int bi = 0; bi < cChunkData::totalSize; bi++)
    { mData.mData[bi].updateOcclusion(); }
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

void cChunk::updateOcclusion(int bx, int by, int bz)
{
  mData.updateOcclusion(bx, by, bz);
}


inline blockSide_t dimSide(int dim, int sign)
{
  int side = dim + (sign < 0 ? 3 : 0);
  return (blockSide_t)(1 << side);
}

inline int getAO(int e1, int e2, int c)
{
  if(e1 == 1 && e2 == 1)
    { return 3; }
  else
    { return (e1 + e2 + c); }
}


void cChunk::updateLighting(int lighting)
{
  //if(isLight())
  //{ return; }

  //LOGD("%d", lighting);
  
  int startZ = sizeZ;
  if(!mNeighbors[blockSide_t::PZ])
    { // nobody above, so assume it's open air.
      startZ--;
      for(int x = 0; x < sizeX; x++)
        for(int y = 0; y < sizeY; y++)
          { mData.mData[cChunkData::index(x, y, startZ)].setLighting(lighting); }
    }
  else
    {
      startZ--;
      for(int x = 0; x < sizeX; x++)
        for(int y = 0; y < sizeY; y++)
          { mData.mData[cChunkData::index(x, y, startZ)].setLighting(mNeighbors[blockSide_t::PZ]->at(Point3i{x, y, 0})->lightLevel); }
    }
  
  std::unordered_set<int> traversed;
  std::vector<Point3i> spread;
  spread.reserve(sizeX*sizeY);
    
  for(int x = 0; x < sizeX; x++)
    for(int y = 0; y < sizeY; y++)
      {
        spread.push_back(Point3i{x, y, startZ});
        traversed.insert(hashBlock(x, y, startZ));
      }

  std::vector<Point3i> nextSpread;
  nextSpread.reserve(sizeX*sizeY);
  while(spread.size() > 0)
    {
      //LOGD("SPREAD SIZE: %d", spread.size());
      for(auto &p : spread)
        {
          cChunk *neighbor = nullptr;
          cBlock *b = at(p, &neighbor);
          if(neighbor)
            { neighbor->setMeshDirty(true); }

          //LOGD("Block: (%d, %d, %d) --> %d : LIGHTING: %d, OCCLUSION: %d", p[0], p[1], p[2], (int)b->type, (int)b->lightLevel, (int)b->occlusion);
          
          // start with adjacent blocks (same z)
          for(int dx = -1; dx <= 1; dx++)
            for(int dy = -1; dy <= 1; dy++)
              {
                Point3i p2{p[0]+dx, p[1]+dy, p[2]};
                if(traversed.count(hashBlock(p2)) == 0)
                  {
                    cBlock *b2 = at(p2);
                    if(b2)
                      {
                        int l = b->lightLevel-1;
                        if(dx && dy)
                          { // account for occlusion from adjacent blocks
                            cBlock *edge1 = at(p[0]+dx, p[1], p[2]);
                            cBlock *edge2 = at(p[0], p[1]+dy, p[2]);
                            int occlusion = 0;
                            if(edge1) { occlusion += edge1->occlusion; }
                            if(edge2) { occlusion += edge2->occlusion; }
                            if(occlusion == 1)
                              { l /= 2; }
                            else if(occlusion == 2)
                              { l = 0; }
                          } // otherwise no occlusion

                        if(b->lightLevel < l)
                        {
                          b2->setLighting(l);
                          if(b2->lightLevel > 0)
                            {
                              nextSpread.push_back(p2);
                              traversed.insert(hashBlock(p2));
                            }
                        }
                      }
                  }
              }
          
          if(b->type != block_t::NONE)
            { continue; }
            
          // now propogate to z-1
          for(int dx = -1; dx <= 1; dx++)
            for(int dy = -1; dy <= 1; dy++)
              {
                Point3i p2{p[0]+dx, p[1]+dy, p[2]-1};
                if(traversed.count(hashBlock(p2)) == 0)
                  {
                    cBlock *b2 = at(p2);
                    if(b2)
                      {
                        int l = b->lightLevel;
                        if(dx && dy)
                          { // account for occlusion from adjacent blocks
                            cBlock *edge1 = at(p[0]+dx, p[1], p[2]);
                            cBlock *edge2 = at(p[0], p[1]+dy, p[2]);
                            cBlock *lowEdge1 = at(p[0]+dx, p[1], p[2]-1);
                            cBlock *lowEdge2 = at(p[0], p[1]+dy, p[2]-1);
                            int occlusion = 0;
                            if(edge1) { occlusion += edge1->occlusion; }
                            if(edge2) { occlusion += edge2->occlusion; }
                            if(lowEdge1) { occlusion += lowEdge1->occlusion; }
                            if(lowEdge2) { occlusion += lowEdge2->occlusion; }
                            if(occlusion == 1)
                              { l -= 2; }
                            else if(occlusion == 2)
                              { l /= 2; }
                            else if(occlusion == 3)
                              { l /= 4; }
                            else if(occlusion == 4)
                              { l = 0; }
                          }
                        else if(dx)
                          {
                            cBlock *edge1 = at(p[0]+dx, p[1], p[2]);
                            if(edge1)
                              { l /= 2; }
                          }
                        else if(dy)
                          {
                            cBlock *edge2 = at(p[0], p[1]+dy, p[2]);
                            if(edge2)
                              { l /= 2; }
                          } // otherwise right below

                        if(b2->lightLevel < l)
                        {
                          b2->setLighting(l);
                          if(b2->lightLevel > 0)
                            {
                              nextSpread.push_back(p2);
                            }
                          traversed.insert(hashBlock(p2));
                        }
                      }
                  }
              }
          
        }
      spread = nextSpread;
      nextSpread.clear();
    }

  setLight(true);
  setMeshDirty(true);

  updateBelow();
}

int8_t cChunk::getLighting(const Point3i &bp, const Point3i &vp, blockSide_t side)
{
  if(!mNeighborsLoaded)
    { return 0; }
  
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
        { lc = adj->occlusion; }
      
      bi[dim1] = bp[dim1];
      adj = at(bi);
      if(adj)
        { le1 = adj->occlusion; }
      
      bi[dim1] = e1;
      bi[dim2] = bp[dim2];
      adj = at(bi);
      if(adj)
        { le2 = adj->occlusion; }
      
      return at(bp)->lightLevel - getAO(le1, le2, lc);
    }
  else
    {
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
                          if(lighting > MAX_LIGHT_LEVEL)
                            { std::cout << lighting << "\n"; }
                          //LOGD("VERTEX -> %f, %f, %f", v.pos[0], v.pos[1], v.pos[2]);
                          mVert.emplace_back(v.pos + vOffset,
                                             v.normal,
                                             v.texcoord,
                                             (int)block->type - 1,
                                             (float)lighting / (float)MAX_LIGHT_LEVEL );
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
  mMeshUploaded = false;
}

void cChunk::uploadMesh()
{
  mMesh->finishUpdating(mInd.size(), mVert, mInd);
}

void cChunk::clearMesh()
{
  mVert.clear();
  mInd.clear();
}


int cChunk::chunkX(int wx)
{ return wx >> cChunkData::shiftX; }
int cChunk::chunkY(int wy)
{ return wy >> cChunkData::shiftY; }
int cChunk::chunkZ(int wz)
{ return wz >> cChunkData::shiftZ; }
Point3i cChunk::chunkPos(const Point3i &wp)
{ return Point3i({chunkX(wp[0]), chunkY(wp[1]), chunkZ(wp[2])}); }


 
inline int cChunk::expand(int x)
{
  x                  &= 0x000003FF;
  x  = (x | (x<<16)) &  0xFF0000FF;
  x  = (x | (x<<8))  &  0x0F00F00F;
  x  = (x | (x<<4))  &  0xC30C30C3;
  x  = (x | (x<<2))  &  0x49249249;
  return x;
}
inline int cChunk::hashBlock(int bx, int by, int bz)
{ return expand(bx) + (expand(by) << 1) + (expand(bz) << 2); }
inline int cChunk::hashBlock(const Point3i &bp)
{ return hashBlock(bp[0], bp[1], bp[2]); }
