#ifndef MEMORY_HPP
#define MEMORY_HPP


#include <cstdint>

class Memory
{
public:
  const int bytes;
  const int speed;
  
  Memory(int numBytes, int memSpeed);
  ~Memory();

  uint8_t access(int addr);
  void set(int addr, uint8_t val);

  void load(int addr, uint8_t *data, int numBytes);
private:
  uint8_t *mData = nullptr;
};


#endif // MEMORY_HPP
