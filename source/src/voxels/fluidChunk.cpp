#include "fluidChunk.hpp"

const Point3i FluidChunk::size {FluidChunk::sizeX, FluidChunk::sizeY, FluidChunk::sizeZ};
const Indexer<FluidChunk::sizeX, FluidChunk::sizeY, FluidChunk::sizeZ> FluidChunk::mIndexer;

FluidChunk::FluidChunk(const Point3i &worldPos)
  : mWorldPos(worldPos), mFluids{{nullptr}}
{ }

FluidChunk::FluidChunk(const Point3i &worldPos, const std::array<Fluid*, FluidChunk::totalSize> &data)
  : mWorldPos(worldPos)
{
  for(int i = 0; i < mFluids.size(); i++)
    {
      if(data[i])
        { mFluids[i] = new Fluid(*data[i]); }
      else
        { mFluids[i] = nullptr; }
    }
}

bool FluidChunk::isEmpty() const
{ return mNumFluids == 0; }
int FluidChunk::numBlocks() const
{ return mNumFluids; }

// Fluid* FluidChunk::at(int hash)
// {
//   auto f = mFluids.find(hash);
//   if(f != mFluids.end())
//     { return f->second; }
//   else
//     { return nullptr; }
// }
Fluid* FluidChunk::at(int bx, int by, int bz)
{
  return mFluids[mIndexer.index(bx, by, bz)];
  /*
  auto f = mFluids.find(Hash::hash(bx, by, bz));
  if(f != mFluids.end())
    { return f->second; }
  else
    { return nullptr; }
  */
}
Fluid* FluidChunk::at(const Point3i &bp)
{
  return mFluids[mIndexer.index(bp)];
  /*
  auto f = mFluids.find(Hash::hash(bp));
  if(f != mFluids.end())
    { return f->second; }
  else
    { return nullptr; }
  */
}
std::array<Fluid*, FluidChunk::totalSize>& FluidChunk::data()
{ return mFluids; }
const std::array<Fluid*, FluidChunk::totalSize>& FluidChunk::data() const
{ return mFluids; }

bool FluidChunk::set(int bx, int by, int bz, Fluid *data)
{
  bool result = false;

  const int bi = mIndexer.index(bx, by, bz);

  if(mFluids[bi])
    {
      result = true;
      if(data)
        {
          *mFluids[bi] = *data;
        }
      else
        {
          delete mFluids[bi];
          mFluids[bi] = nullptr;
          mNumFluids--;
        }
    }
  else if(data)
    {
      result = true;
      mFluids[bi] = new Fluid(*data);
      mNumFluids++;
    }

  return result;
  /*
  bool result = false;
  const int hash = Hash::hash(bx, by, bz);

  auto iter = mFluids.find(hash);
  if(iter != mFluids.end())
    { // remove/replace data
      result = true;
      if(data)
        { // copy data
          *iter->second = *data;
        }
      else
        { // delete data
          delete iter->second;
          mFluids.erase(iter->first);
        }
    }
  else
    {
      if(data)
        {
          mFluids[hash] = (Fluid*)data->copy();
          result = true;
        }
      else
        {
          result = false;
        }
    }
  return result;
  */
}
bool FluidChunk::set(const Point3i &bp, Fluid *data)
{
  return set(bp[0], bp[1], bp[2], data);
}

void FluidChunk::reset()
{
  for(int i = 0; i < totalSize; i++)
    {
      if(mFluids[i])
        {
          delete mFluids[i];
          mFluids[i] = nullptr;
        }
    }
  mNumFluids = 0;
  // for(auto &f : mFluids)
  //   {
  //     delete f.second;
  //   }
  // mFluids.clear();
}

bool FluidChunk::step(float evap)
{
  bool result = false;
  for(int i = 0; i < totalSize; i++)
    {
      if(mFluids[i])
        {
          result |= mFluids[i]->step(evap);
          if(mFluids[i]->isEmpty())
            {
              result = true;
              delete mFluids[i];
              mFluids[i] = nullptr;
              mNumFluids--;
            }
        }
    }
  return result;
}

int FluidChunk::serialize(std::vector<uint8_t> &dataOut) const
{
  /*
  static const int fluidSize = Fluid::dataSize + sizeof(int32_t);
  dataOut.resize(mFluids.size() * fluidSize);
  int offset = 0;
  for(auto &f : mFluids)
    {
      std::memcpy((void*)&f.first, (void*)&dataOut[offset], sizeof(int32_t));
      offset += sizeof(int32_t);
      offset += reinterpret_cast<Fluid*>(f.second)->serialize(&dataOut[offset]);
    }
  return offset;
  */
  return 0;
}
void FluidChunk::deserialize(const std::vector<uint8_t> &dataIn)
{
  /*
  static const int fluidSize = Fluid::dataSize + sizeof(int32_t);
  reset();
  
  int offset = 0;
  while(offset + fluidSize <= dataIn.size())
    {
      Fluid *data = new Fluid(&dataIn[offset]);
      offset += fluidSize;
    }
  mDirty = true;
  */
}
