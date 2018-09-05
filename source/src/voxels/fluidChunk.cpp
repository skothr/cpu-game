#include "fluidChunk.hpp"

const Point3i FluidChunk::size {FluidChunk::sizeX, FluidChunk::sizeY, FluidChunk::sizeZ};

FluidChunk::FluidChunk(const Point3i &worldPos)
  : mWorldPos(worldPos)
{ }

FluidChunk::FluidChunk(const Point3i &worldPos, const std::unordered_map<int32_t, Fluid*> &data)
  : mWorldPos(worldPos)
{
  for(auto iter : data)
    {
      if(iter.second)
        {
          mFluids.emplace(iter.first, new Fluid(*iter.second));
        }
    }
}


bool FluidChunk::isEmpty() const
{ return mFluids.size() == 0; }
int FluidChunk::numBlocks() const
{ return mFluids.size(); }

Fluid* FluidChunk::at(int hash)
{
  auto f = mFluids.find(hash);
  if(f != mFluids.end())
    { return f->second; }
  else
    { return nullptr; }
}
Fluid* FluidChunk::at(int bx, int by, int bz)
{
  auto f = mFluids.find(Hash::hash(bx, by, bz));
  if(f != mFluids.end())
    { return f->second; }
  else
    { return nullptr; }
}
Fluid* FluidChunk::at(const Point3i &bp)
{
  auto f = mFluids.find(Hash::hash(bp));
  if(f != mFluids.end())
    { return f->second; }
  else
    { return nullptr; }
}
std::unordered_map<int32_t, Fluid*>& FluidChunk::data()
{ return mFluids; }
const std::unordered_map<int32_t, Fluid*>& FluidChunk::data() const
{ return mFluids; }

bool FluidChunk::set(int bx, int by, int bz, Fluid *data)
{
  bool result = false;
  const int hash = Hash::hash(bx, by, bz);

  //if(data)
  //  { LOGD("Adding fluid --> %f %f", data->fluidEvap, data->fluidLevel); }

  
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
}
bool FluidChunk::set(const Point3i &bp, Fluid *data)
{
  return set(bp[0], bp[1], bp[2], data);
}

void FluidChunk::reset()
{
  for(auto &f : mFluids)
    {
      delete f.second;
    }
  mFluids.clear();
}

// std::unordered_map<int, Fluid*> FluidChunk::getFluids()
// {
//   return mFluids;
// }

bool FluidChunk::step(bool evap)
{
  bool result = false;
  std::vector<int32_t> gone;
  for(auto &f : mFluids)
    {
      if(evap)
        { result |= f.second->step(); }
      if(f.second->isEmpty())
        {
          delete f.second;
          f.second = nullptr;
          gone.push_back(f.first);
        }
    }
  for(auto hash : gone)
    { mFluids.erase(hash); }
  return result | gone.size() > 0;
}

int FluidChunk::serialize(std::vector<uint8_t> &dataOut) const
{
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
}
void FluidChunk::deserialize(const std::vector<uint8_t> &dataIn)
{
  static const int fluidSize = Fluid::dataSize + sizeof(int32_t);
  reset();
  
  int offset = 0;
  while(offset + fluidSize <= dataIn.size())
    {
      Fluid *data = new Fluid(&dataIn[offset]);
      offset += fluidSize;
    }
  mDirty = true;
}
