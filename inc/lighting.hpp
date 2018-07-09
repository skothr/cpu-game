#ifndef LIGHTING_HPP
#define LIGHTING_HPP


class cBlockLight
{
public:
  cBlockLight()
    : mParts{0,0,0,0}
  { }

  // mParts[0] --> RED
  // mParts[1] --> GREEN
  // mParts[2] --> BLUE
  // mParts[3] --> BRIGHTNESS SCALAR
  uint8_t mParts[4];

  bool brighterThan(const cBlockLight &other)
  {
    
  }
  
private:
  
};


#endif // LIGHTING_HPP
