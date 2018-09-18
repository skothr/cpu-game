#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <array>
#include <cmath>
#include <ostream>
#include <type_traits>
#include <initializer_list>
#include <cassert>
#include <cmath>
#include <algorithm>

#include "logging.hpp"

template<typename T, int N>
class Vector
{
private:
  T mData[N];

public:
  Vector();
  Vector(const std::initializer_list<T> &values);
  //Vector(const std::array<T, N> &values);
  Vector(const Vector<T, N> &other);
  template<int M>
  Vector(const Vector<T, M> &other);
  template<typename U>
  Vector(const Vector<U, N> &other);
  
  template<int M>
  Vector<T, N>& operator=(const Vector<T, M> &other);

  bool operator==(const Vector<T, N> &other) const;
  bool operator!=(const Vector<T, N> &other) const;

  T max() const
  { return std::max_element(&mData[0], &mData[N-1]); }
  T min() const
  { return std::min_element(&mData[0], &mData[N-1]); }

  Vector<T, N+1> asPoint() const
  {
    Vector<T, N+1> v = *this;
    v[N] = 1;
    return v;
  }
  Vector<T, N+1> asVector() const
  {
    Vector<T, N+1> v = *this;
    v[N] = 0;
    return v;
  }

  T& at(int i)
  { return mData[i]; }
  const T& at(int i) const
  { return mData[i]; }

  T& operator[](int i);
  const T& operator[](int i) const;
  T& operator()(int i);
  const T& operator()(int i) const;

  const T* data() const;
  T* data();

  T* begin()
  { return &mData[0]; }
  const T* begin() const
  { return &mData[0]; }
  T* end()
  { return &mData[N]; }
  const T* end() const
  { return &mData[N]; }
  
  Vector<T, N>& operator+=(const Vector<T, N> &other);
  Vector<T, N> operator+(const Vector<T, N> &other) const;
  Vector<T, N>& operator-=(const Vector<T, N> &other);
  Vector<T, N> operator-(const Vector<T, N> &other) const;
  Vector<T, N>& operator+=(const T &scalar);
  Vector<T, N> operator+(const T &scalar) const;
  Vector<T, N>& operator-=(const T &scalar);
  Vector<T, N> operator-(const T &scalar) const;
  
  Vector<T, N>& operator*=(const T &scalar);
  Vector<T, N> operator*(const T &scalar) const;
  Vector<T, N>& operator/=(const T &scalar);
  Vector<T, N> operator/(const T &scalar) const;
  Vector<T, N>& operator%=(const T &scalar);
  Vector<T, N> operator%(const T &scalar) const;
  Vector<T, N>& operator%=(const Vector<T, N> &other);
  Vector<T, N> operator%(const Vector<T, N> &other) const;

  // TODO: Restrict these to int vectors
  Vector<T, N>& operator&=(const T &scalar);
  Vector<T, N> operator&(const T &scalar) const;
  Vector<T, N>& operator|=(const T &scalar);
  Vector<T, N> operator|(const T &scalar) const;
  Vector<T, N>& operator&=(const Vector<T, N> &other);
  Vector<T, N> operator&(const Vector<T, N> &other) const;
  Vector<T, N>& operator|=(const Vector<T, N> &other);
  Vector<T, N> operator|(const Vector<T, N> &other) const;
  
  Vector<T, N> operator<<(const T &scalar) const;
  Vector<T, N> operator>>(const T &scalar) const;
  Vector<T, N> operator<<(const Vector<T, N> &other) const;
  Vector<T, N> operator>>(const Vector<T, N> &other) const;
  
  Vector<T, N>& operator*=(const Vector<T, N> &other);
  Vector<T, N> operator*(const Vector<T, N> &other) const;

  Vector<T, N>& operator-()
  {
    for(int i = 0; i < N; i++)
      { mData[i] = -mData[i]; }
    return *this;
  }
  Vector<T, N> operator-() const
  {
    return -Vector<T, N>(*this);
  }

  /*
  Vector<T, N> intersection(const Vector &min1, const Vector &max1,
                            const Vector &min2, const Vector &max2) const
  {
    return Vector({min1[0] <});
  }
  */

  T length() const;
  Vector<T, N> normalized() const;
  T dot(const Vector<T, N> &other) const;
  Vector<T, N> reflect(const Vector<T, N> &normal) const;
  Vector<T, N> abs() const;
  /*
  typename std::enable_if<(N==3), Vector<T, N>>::type cross(const Vector<T, N> &other) const
  {
    return Vector<T, N>(
      {   mData[1]*other.mData[2] - mData[2]*other.mData[1],
	  mData[2]*other.mData[0] - mData[0]*other.mData[2],
	  mData[0]*other.mData[1] - mData[1]*other.mData[0] } );
  }
  */
  
  template<typename U, int M>
  friend std::ostream& operator<<(std::ostream &os, const Vector<U, M> &v);
};
  

template<typename T, int N>
Vector<T, N>::Vector()
{
  memset(mData, 0, sizeof(T)*N);
}
template<typename T, int N>
Vector<T, N>::Vector(const std::initializer_list<T> &values)
{
  assert(values.size() == N);
  std::copy(values.begin(), values.end(), mData);
}
template<typename T, int N>
Vector<T, N>::Vector(const Vector<T, N> &other)
{
  //LOGD("TEST1");
  std::copy(other.mData, other.mData + N, &mData[0]);
  //LOGD("TEST2");
}

template<typename T, int N>
template<int M>
Vector<T, N>::Vector(const Vector<T, M> &other)
{
  //LOGD("vectorN(vectorN-1)");
  if(M < N)
    {
      /*
      std::copy(&other[0], &other[M], &mData[0]);
      memset(&mData[M], 0, (N-M)*sizeof(T));
    */
    
    memset(mData, 0, sizeof(T)*N);
    for(int i = 0; i < M; i++)
      { mData[i] = other[i]; }
    }
  else
    {
      std::copy(&other[0], &other[N], &mData[0]);
    }
  //LOGD("vectorN(vectorN-1) DONE");
  //std::copy(other.mData, &other.mData[N], mData);
}

template<typename T, int N>
template<typename U>
Vector<T, N>::Vector(const Vector<U, N> &other)
{
  for(int i = 0; i < N; i++)
    { mData[i] = (T)other.data()[i]; }
}
template<typename T, int N>
template<int M>
Vector<T, N>& Vector<T, N>::operator=(const Vector<T, M> &other)
{
  //LOGD("vector%d = vector%d", N, M);
  if(M <= N)
    {
      //std::copy(&other[0], &other[M], &mData[0]);
      //memset(&mData[M], 0, (N-M)*sizeof(T));
      
      memset(mData, 0, sizeof(T)*N);
      for(int i = 0; i < M; i++)
	{ mData[i] = other[i]; }
    }
  else
    {
      // std::copy(&other[0], &other[N], &mData[0]);
      for(int i = 0; i < N; i++)
      	{ mData[i] = other[i]; }
      
    }
  //LOGD("vectorN = vectorN-1 DONE");
  return *this;
}


template<typename T, int N>
T& Vector<T, N>::operator[](int i)
{
  return mData[i];
}
template<typename T, int N>
const T& Vector<T, N>::operator[](int i) const
{
  return mData[i];
}
template<typename T, int N>
T& Vector<T, N>::operator()(int i)
{
  return mData[i];
}
template<typename T, int N>
const T& Vector<T, N>::operator()(int i) const
{
  return mData[i];
}


template<typename T, int N>
bool Vector<T, N>::operator==(const Vector<T, N> &other) const
{
  for(int i = 0; i < N; i++)
    {
      if(mData[i] != other.mData[i])
	{ return false; }
    }
  return true;
}

template<typename T, int N>
bool Vector<T, N>::operator!=(const Vector<T, N> &other) const
{
  return !((*this) == other);
}

template<typename T, int N>
const T* Vector<T, N>::data() const
{ return mData; }
template<typename T, int N>
T* Vector<T, N>::data()
{ return mData; }

template<typename T, int N>
Vector<T, N>& Vector<T, N>::operator+=(const Vector<T, N> &other)
{
  for(int i = 0; i < N; i++)
    { mData[i] += other.mData[i]; }
  return *this;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator+(const Vector<T, N> &other) const
{
  Vector<T, N> result(*this);
  return result += other;
}
template<typename T, int N>
Vector<T, N>& Vector<T, N>::operator-=(const Vector<T, N> &other)
{
  for(int i = 0; i < N; i++)
    { mData[i] -= other.mData[i]; }
  return *this;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator-(const Vector<T, N> &other) const
{
  Vector<T, N> result(*this);
  return result -= other;
}


template<typename T, int N>
  Vector<T, N>& Vector<T, N>::operator+=(const T &scalar)
{
  for(int i = 0; i < N; i++)
    { mData[i] += scalar; }
  return *this;
}
template<typename T, int N>
  Vector<T, N> Vector<T, N>::operator+(const T &scalar) const
{
  Vector<T, N> result(*this);
  return result += scalar;
}
template<typename T, int N>
  Vector<T, N>& Vector<T, N>::operator-=(const T &scalar)
{
  for(int i = 0; i < N; i++)
    { mData[i] -= scalar; }
  return *this;
}
template<typename T, int N>
  Vector<T, N> Vector<T, N>::operator-(const T &scalar) const
{
  Vector<T, N> result(*this);
  return result -= scalar;
}


template<typename T, int N>
Vector<T, N>& Vector<T, N>::operator*=(const T &scalar)
{
  for(int i = 0; i < N; i++)
    { mData[i] *= scalar; }
  return *this;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator*(const T &scalar) const
{
  Vector<T, N> result(*this);
  return result *= scalar;
}
template<typename T, int N>
Vector<T, N>& Vector<T, N>::operator/=(const T &scalar)
{
  for(int i = 0; i < N; i++)
    { mData[i] /= scalar; }
  return *this;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator/(const T &scalar) const
{
  Vector<T, N> result(*this);
  return result /= scalar;
}
template<typename T, int N>
Vector<T, N>& Vector<T, N>::operator%=(const T &scalar)
{
  for(int i = 0; i < N; i++)
    { mData[i] %= scalar; }
  return *this;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator%(const T &scalar) const
{
  Vector<T, N> result(*this);
  return result %= scalar;
}
template<typename T, int N>
Vector<T, N>& Vector<T, N>::operator%=(const Vector<T, N> &other)
{
  for(int i = 0; i < N; i++)
    { mData[i] %= other[i]; }
  return *this;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator%(const Vector<T, N> &other) const
{
  Vector<T, N> result(*this);
  return result %= other;
}


template<typename T, int N>
Vector<T, N>& Vector<T, N>::operator&=(const T &scalar)
{
  for(int i = 0; i < N; i++)
    { mData[i] &= scalar; }
  return *this;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator&(const T &scalar) const
{
  Vector<T, N> result(*this);
  return result &= scalar;
}
template<typename T, int N>
Vector<T, N>& Vector<T, N>::operator|=(const T &scalar)
{
  for(int i = 0; i < N; i++)
    { mData[i] |= scalar; }
  return *this;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator|(const T &scalar) const
{
  Vector<T, N> result(*this);
  return result |= scalar;
}

template<typename T, int N>
Vector<T, N>& Vector<T, N>::operator&=(const Vector<T, N> &other)
{
  for(int i = 0; i < N; i++)
    { mData[i] &= other.mData[i]; }
  return *this;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator&(const Vector<T, N> &other) const
{
  Vector<T, N> result(*this);
  return result &= other;
}
template<typename T, int N>
Vector<T, N>& Vector<T, N>::operator|=(const Vector<T, N> &other)
{
  for(int i = 0; i < N; i++)
    { mData[i] |= other.mData[i]; }
  return *this;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator|(const Vector<T, N> &other) const
{
  Vector<T, N> result(*this);
  return result |= other;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator<<(const T &scalar) const
{
  Vector<T, N> result;
  for(int i = 0; i < N; i++)
    { result[i] = mData[i] << scalar; }
  return result;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator>>(const T &scalar) const
{
  Vector<T, N> result;
  for(int i = 0; i < N; i++)
    { result[i] = mData[i] >> scalar; }
  return result;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator<<(const Vector<T, N> &other) const
{
  Vector<T, N> result;
  for(int i = 0; i < N; i++)
    { result[i] = mData[i] << other.mData[i]; }
  return result;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator>>(const Vector<T, N> &other) const
{
  Vector<T, N> result;
  for(int i = 0; i < N; i++)
    { result[i] = mData[i] >> other.mData[i]; }
  return result;
}

template<typename T, int N>
Vector<T, N>& Vector<T, N>::operator*=(const Vector<T, N> &other)
{
  for(int i = 0; i < N; i++)
    { mData[i] *= other.mData[i]; }
  return *this;
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::operator*(const Vector<T, N> &other) const
{
  Vector<T, N> result(*this);
  return result *= other;
}

template<typename T, int N>
T Vector<T, N>::length() const
{
  T mag = 0.0;
  for(int i = 0; i < N; i++)
    { mag += mData[i]*mData[i]; }
  return sqrt(mag);
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::normalized() const
{
  T len = length();
  return (*this) / std::max(len, (T)0.01);
}

template<typename T, int N>
T Vector<T, N>::dot(const Vector<T, N> &other) const
{
  T result = (T)0;
  for(int i = 0; i < N; i++)
    { result += mData[i] * other.mData[i]; }
  return result;
}

template<typename T, int N>
Vector<T, N> Vector<T, N>::reflect(const Vector<T, N> &normal) const
{
  return (*this) - (normal * (T)2.0 * this->dot(normal));
}
template<typename T, int N>
Vector<T, N> Vector<T, N>::abs() const
{
  Vector<T, N> result;
  for(int i = 0; i < N; i++)
    { result[i] = std::abs(result[i]); }
  return result;
}

template<typename T, int N>
std::ostream& operator<<(std::ostream &os, const Vector<T, N> &v)
{
  os << "[ ";
  for(int i = 0; i < N; i++)
    { os << v.mData[i] << " "; }
  os << "]";
  return os;
}


template<typename T>
Vector<T, 3> crossProduct(const Vector<T, 3> &v1, const Vector<T, 3> &v2)
{
  return Vector<T, 3>{  v1[1]*v2[2] - v1[2]*v2[1],
			v1[2]*v2[0] - v1[0]*v2[2],
			v1[0]*v2[1] - v1[1]*v2[0] };
}


typedef Vector<float, 1> Vector1f;
typedef Vector<float, 2> Vector2f;
typedef Vector<float, 3> Vector3f;
typedef Vector<float, 4> Vector4f;
typedef Vector<double, 1> Vector1d;
typedef Vector<double, 2> Vector2d;
typedef Vector<double, 3> Vector3d;
typedef Vector<double, 4> Vector4d;
typedef Vector<int, 1> Vector1i;
typedef Vector<int, 2> Vector2i;
typedef Vector<int, 3> Vector3i;
typedef Vector<int, 4> Vector4i;

typedef Vector1f Point1f;
typedef Vector2f Point2f;
typedef Vector3f Point3f;
typedef Vector4f Point4f;
typedef Vector1d Point1d;
typedef Vector2d Point2d;
typedef Vector3d Point3d;
typedef Vector4d Point4d;
typedef Vector1i Point1i;
typedef Vector2i Point2i;
typedef Vector3i Point3i;
typedef Vector4i Point4i;


inline Vector3f rotateVec(const Vector3f v, const Vector3f ax, float angle)
{
  float cosa = cos(angle);
  float sina = sin(angle);
  return (v * cosa) + (crossProduct(ax, v) * sina) + (ax * ax.dot(v) * (1.0f - cosa));
}


#include <QVector3D>

static QVector3D toQt(const Vector<float, 3> &v)
{ return QVector3D(v[0], v[1], v[2]); }

#endif //VECTOR_HPP
