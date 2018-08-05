#ifndef HELPERS_HPP
#define HELPERS_HPP

#define ENUM_CLASS_BITWISE_OPERATORS(type) \
  inline type& operator|=(type &u1, const type &u2) { u1 = (type)((int)u1 | (int)u2); return u1;} \
  inline type operator|(type u1, type u2) { return (type)((int)u1 | (int)u2); } \
  inline type& operator&=(type &u1, const type &u2) { u1 = (type)((int)u1 & (int)u2); return u1; } \
  inline type operator&(type u1, type u2) { return (type)((int)u1 & (int)u2); } \
  inline type operator~(type u1) { return (type)~((int)u1); } \
  inline int toInt(type u1) { return (int)u1; }		      \
  inline bool toBool(type u1) { return (u1 != (type)0); } 

#endif // HELPERS_HPP
