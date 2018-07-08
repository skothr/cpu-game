#ifndef MEMORY_HPP
#define MEMORY_HPP


#include <cstdint>

class cMemory
{
public:
  const int bytes;
  const int speed;
  
  cMemory(int numBytes, int memSpeed);
  ~cMemory();

  uint8_t access(int addr);
  void set(int addr, uint8_t val);

  void load(int addr, uint8_t *data, int numBytes);
private:
  uint8_t *mData = nullptr;
};


#endif // MEMORY_HPP
