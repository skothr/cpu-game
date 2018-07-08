#include "octree.hpp"

#include "logging.hpp"
#include <cstring>

cOctree::cOctree(cOctree *parent, const Point3i &origin, const Vector3i &size)
  : mParent(parent), mOrigin(origin), mSize(size)
{
  //std::memset(mChildren, 0, sizeof(mChildren));
  for(int i = 0; i < 8; i++)
    { mChildren[i] = nullptr; }
}

cOctree::~cOctree()
{
  if(mData)
    { delete mData; }
  if(mPos)
    { delete mPos; }
  if(!isLeaf())
    {
      for(int i = 0; i < 8; i++)
	{ delete mChildren[i]; }
    }
}


int cOctree::size() const
{
  if(isLeaf())
    { return mData ? 1 : 0; }
  else
    {
      int sum = 0;
      for(int i = 0; i < 8; i++)
	{ sum += mChildren[i]->size(); }
      return sum;
    }
}

// Determine which octant of the tree would contain 'point'
int cOctree::getOctant(const Point3i *p) const
{
  // TODO: Optimize with bit-shifts?
  int oct = 0x00;
  if((*p)[0] >= mOrigin[0]) oct |= 0x04;
  if((*p)[1] >= mOrigin[1]) oct |= 0x02;
  if((*p)[2] >= mOrigin[2]) oct |= 0x01;
  return oct;
}

void cOctree::insert(const Point3i &p, block_t type)
{
  insert(new Point3i(p), new cBlock(type));
}

void cOctree::insert(const Point3i &p, cBlock *block)
{
  insert(new Point3i(p), block);
}

void cOctree::insert(Point3i *p, cBlock *b)
{
  if(isLeaf())
    {
      if(!mData)
	{
	  mData = b;
	  mPos = p;
	}
      else
	{
	  if(mSize[0] >= 1)
	    {
	      cBlock *old = mData;
	      Point3i *oldP = mPos;
	      mData = nullptr;
	      mPos = nullptr;
	      for(int i = 0; i < 8; i++)
		{
		  // Compute new bounding box for this child
		  Point3i newOrigin = mOrigin;
		  newOrigin[0] += mSize[0] / ((i & 0x04) ? 4 : -4);
		  newOrigin[1] += mSize[1] / ((i & 0x02) ? 4 : -4);
		  newOrigin[2] += mSize[2] / ((i & 0x01) ? 4 : -4);
		  //LOGD("new origin: %f, %f, %f", newOrigin[0], newOrigin[1], newOrigin[2]);
		  mChildren[i] = new cOctree(this, newOrigin, mSize/2);
		}
	      //LOGD("Inserting old...");
	      mChildren[getOctant(oldP)]->insert(oldP, old);
	      //LOGD("Inserting new...");
	      mChildren[getOctant(p)]->insert(p, b);
	    }
	  else
	    {
	      LOGW("Octree insert --> identical point inserted! s=%d", mSize[0]);
	      std::cout << "Point: " << p << "\n";
	    }
	}
    }
  else
    {
      //LOGD("Next child (%d)...", getOctant(p));
      mChildren[getOctant(p)]->insert(p, b);
    }
}

void cOctree::remove(const Point3i &p, bool keep)
{
  if(mData)
    {
      if(p == *mPos)
	{
	  if(!keep)
	    {
	      delete mData;
	      delete mPos;
	    }
	  mData = nullptr;
	  mPos = nullptr;
	}
    }
  else if(!isLeaf())
    {
      if(!mChildren[getOctant(&p)]->isLeaf() || mChildren[getOctant(&p)]->mData)
	{
	  mChildren[getOctant(&p)]->remove(p, keep);
      
	  bool others = false;
	  for(int i = 0; i < 8; i++)
	    {
	      if(!mChildren[i]->isLeaf() || mChildren[i]->mData)
		{
		  others = true;
		  break;
		}
	    }
      
	  if(!others)
	    {
	      for(int i = 0; i < 8; i++)
		{
		  delete mChildren[i];
		  mChildren[i] = nullptr;
		}
	    }
	}
    }
}

bool cOctree::isLeaf() const
{
  return mChildren[0] == nullptr;
}

bool cOctree::iterate(OctreeIterFunc f)
{
  if(mData)
    {
      if(!f(mPos, mData))
	{ return false; }
    }

  if(!isLeaf())
    {
      for(int i = 0; i < 8; i++)
	{
	  if(!mChildren[i]->iterate(f))
	    return false;
	}
    }
  return true;
}

bool cOctree::contains(const Point3i &p) const
{
  if(mData && *mPos == p)
    { return true; }
  else if(!isLeaf())
    {
      for(int i = 0; i < 8; i++)
	{ if(mChildren[i]->contains(p)) return true; }
    }
  return false;
}

cBlock* cOctree::get(const Point3i &p) const
{
  if(mData && *mPos == p)
    { return mData; }
  else if(!isLeaf())
    {
      for(int i = 0; i < 8; i++)
	{
	  cBlock *b = mChildren[i]->get(p);
	  if(b) return b;
	}
    }
  return nullptr;
}

block_t cOctree::getType(const Point3i &p) const
{
  if(mData && *mPos == p)
    { return mData->type; }
  else if(!isLeaf())
    {
      for(int i = 0; i < 8; i++)
	{
	  block_t t = mChildren[i]->getType(p);
	  if(t != block_t::NONE) return t;
	}
    }
  return block_t::NONE;
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
bool cOctree::getClosestBlock(const Point3f &p, const Vector3f &d, float radius, cBlock *&blockOut, Point3i &posOut, Vector3i &faceOut)
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
  
  while((step[0] > 0 ? (pi[0] < mSize[0]) : (pi[0] >= 0)) &&
	(step[1] > 0 ? (pi[1] < mSize[1]) : (pi[1] >= 0)) &&
	(step[2] > 0 ? (pi[2] < mSize[2]) : (pi[2] >= 0)) )
    {
      if(!(pi[0] < 0 || pi[1] < 0 || pi[2] < 0 || pi[0] >= mSize[0] || pi[1] >= mSize[1] || pi[2] >= mSize[2]))
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


void cOctree::getPointsInBox(const Point3f &min, const Point3f &max, std::vector<Point3i*> &pOut, std::vector<cBlock*> &bOut) const
{
  if(isLeaf())
    {
      if(mData)
	{
	  const Point3i &p = *mPos;
	  if(p[0] > max[0] || p[1] > max[1] || p[2] > max[2]) return;
	  if(p[0] < min[0] || p[1] < min[1] || p[2] < min[2]) return;
	  pOut.push_back(mPos);
	  bOut.push_back(mData);
	}
    }
  else
    {
      for(int i = 0; i < 8; i++)
	{
	  Point3f cmin = mChildren[i]->mOrigin - mChildren[i]->mSize/2;
	  Point3f cmax = mChildren[i]->mOrigin + mChildren[i]->mSize/2;

	  if(cmin[0] > max[0] || cmin[1] > max[1] || cmin[2] > max[2]) continue;
	  if(cmax[0] < min[0] || cmax[1] < min[1] || cmax[2] < min[2]) continue;

	  mChildren[i]->getPointsInBox(min, max, pOut, bOut);
	}
    }
}



blockSide_t cOctree::adjacent(const Point3i &p) const
{
  if(!isLeaf())
    {
      Point3f ppx{p[0]+1, p[1], p[2]};
      Point3f pnx{p[0]-1, p[1], p[2]};
      Point3f ppy{p[0], p[1]+1, p[2]};
      Point3f pny{p[0], p[1]-1, p[2]};
      Point3f ppz{p[0], p[1], p[2]+1};
      Point3f pnz{p[0], p[1], p[2]-1};

      blockSide_t flags = BS_NONE;
      if(contains(ppx))
	{ flags |= BS_POSX; }
      if(contains(pnx))
	{ flags |= BS_NEGX; }
      if(contains(ppy))
	{ flags |= BS_POSY; }
      if(contains(pny))
	{ flags |= BS_NEGY; }
      if(contains(ppz))
	{ flags |= BS_POSZ; }
      if(contains(pnz))
	{ flags |= BS_NEGZ; }
      return flags;
    }
  return BS_INVALID;
}

blockSide_t cOctree::getAdjacent(const Point3i &p, std::vector<cBlock*> &blocksOut) const
{
  if(!isLeaf())
    {
      Point3f ppx{p[0]+1, p[1], p[2]};
      Point3f pnx{p[0]-1, p[1], p[2]};
      Point3f ppy{p[0], p[1]+1, p[2]};
      Point3f pny{p[0], p[1]-1, p[2]};
      Point3f ppz{p[0], p[1], p[2]+1};
      Point3f pnz{p[0], p[1], p[2]-1};

      blockSide_t flags = BS_NONE;
      cBlock *b;
      if(b = get(ppx))
	{
	  blocksOut.push_back(b);
	  flags |= BS_POSX;
	}
      if(b = get(pnx))
	{
	  blocksOut.push_back(b);
	  flags |= BS_NEGX;
	}
      if(b = get(ppy))
	{
	  blocksOut.push_back(b);
	  flags |= BS_POSY;
	}
      if(b = get(pny))
	{
	  blocksOut.push_back(b);
	  flags |= BS_NEGY;
	}
      if(b = get(ppz))
	{
	  blocksOut.push_back(b);
	  flags |= BS_POSZ;
	}
      if(b = get(pnz))
	{
	  blocksOut.push_back(b);
	  flags |= BS_NEGZ;
	}
      return flags;
    }
  return BS_INVALID;
}

void cOctree::getAll(std::vector<Point3i*> &pointsOut, std::vector<cBlock*> &blocksOut) const
{
  if(mData)
    {
      pointsOut.push_back(mPos);
      blocksOut.push_back(mData);
    }
  else if(!isLeaf())
    {
      for(int i = 0; i < 8; i++)
	{
	  mChildren[i]->getAll(pointsOut, blocksOut);
	}
    }
}

std::ostream& operator<<(std::ostream &os, const cOctree &oct)
{
  os << "OCTREE\n";
  oct.print(os, 0);
  return os;
}

void cOctree::print(std::ostream &os, int depth) const
{
  if(mData)
    {
      for(int i = 0; i < depth; i++)
	{ os << "-"; }
      os << ">[LEAF: " << (*mPos) << " / " << (int)mData->type << "]\n";
    } 
  else if(!isLeaf())
    {
      for(int i = 0; i < depth; i++)
	{ os << "-"; }
      os << ">[PARENT]\n";
      
      for(int i = 0; i < 8; i++)
	{
	  mChildren[i]->print(os, depth+1);
	}
    }
}
