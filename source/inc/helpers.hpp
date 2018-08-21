#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <string>
#include <iostream>

#define ENUM_CLASS_BITWISE_OPERATORS(type) \
  inline type& operator|=(type &u1, const type &u2) { u1 = (type)((int)u1 | (int)u2); return u1;} \
  inline type operator|(type u1, type u2) { return (type)((int)u1 | (int)u2); } \
  inline type& operator&=(type &u1, const type &u2) { u1 = (type)((int)u1 & (int)u2); return u1; } \
  inline type operator&(type u1, type u2) { return (type)((int)u1 & (int)u2); } \
  inline type operator~(type u1) { return (type)~((int)u1); } \
  inline int toInt(type u1) { return (int)u1; }		      \
  inline bool toBool(type u1) { return (u1 != (type)0); }


static bool promptUserYN(const std::string &prompt, bool defaultChoice = true)
{
  std::cout << prompt << " (" << (defaultChoice ? "Y" : "y") << "/"
            << (defaultChoice ? "n" : "N") << ")   ";
  bool confirm = false;
  while(true)
    {
      std::string resp;
      std::cin >> resp;
      if(resp[0] == 'y' || resp[0] == 'Y' || defaultChoice && resp == "")
        {
          confirm = true;
          break;
        }
      else if(resp[0] == 'n' || resp[0] == 'N' || !defaultChoice && resp == "")
        {
          confirm = false;
          break;
        }
      else
        {
          std::cout << "Please enter 'y' or 'n' (default "
                    << (defaultChoice ? "Yes" : "No") << ")   ";
        }
    }
  return confirm;
}











#endif // HELPERS_HPP
