#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <iostream>
#include <iomanip>
#include <cstring>
#include "vector.hpp"
#include "logging.hpp"

#include <QMatrix4x4>




// Graphics matrices

template<int N>
class Matrix
{
public:
  Matrix(const float *pData = nullptr);
  Matrix(const Matrix<N> &other);
  Matrix(const QMatrix4x4 &other);
  //~Matrix();
  
  Matrix<N>& operator=(const Matrix<N> &other);

  Matrix<N> transposed() const;
  
private:
  static const int elemSize = N * N;
  static const int dataSize = N * N * sizeof(float);
  //float mData[N][N];
  float *mData = nullptr;//[N*N];
  //unsigned int calcIndex(int row, int col) const;

public:
  // data access
  float& operator()(unsigned int r, unsigned int c);
  float operator()(unsigned int r, unsigned int c) const;
  float& at(unsigned int r, unsigned int c)
  { return mData[r*N+c]; }
  float at(unsigned int r, unsigned int c) const
  { return mData[r*N+c]; }

  const float* data() const;
  float* dataCopy() const;
  //const float data()[4][4] const;

  // operations
  Matrix<N>& operator*=(const Matrix<N> &other);
  Matrix<N> operator*(const Matrix<N> &other) const;
  Matrix<N>& operator+=(const Matrix<N> &other);
  Matrix<N> operator+(const Matrix<N> &other) const;
  Matrix<N>& operator-=(const Matrix<N> &other);
  Matrix<N> operator-(const Matrix<N> &other) const;
  
  Vector<float, N> operator*(const Vector<float, N> &v) const;
  friend Vector<float, N> operator*(const Vector<float, N> &v, const Matrix<N> &m)
  {
    return Vector<float, 4>({m.at(0,0)*v[0] + m.at(1,0)*v[1] + m.at(2,0)*v[2] + m.at(3,0)*v[3],
                             m.at(0,1)*v[0] + m.at(1,1)*v[1] + m.at(2,1)*v[2] + m.at(3,1)*v[3],
                             m.at(0,2)*v[0] + m.at(1,2)*v[1] + m.at(2,2)*v[2] + m.at(3,2)*v[3],
                             m.at(0,3)*v[0] + m.at(1,3)*v[1] + m.at(2,3)*v[2] + m.at(3,3)*v[3] });
  }

  Matrix<N>& operator*=(const float &scalar);
  Matrix<N> operator*(const float &scalar) const;
  ///Matrix<N> operator*(const float &scalar, const Matrix<N> &mat) const;
  Matrix<N>& operator/=(const float &scalar);
  Matrix<N> operator/(const float &scalar) const;

  Matrix<N> cross(const Matrix<4> &other) const;

  QMatrix4x4 getQMat() const
  {
    return QMatrix4x4(mData);
  }
  
  Matrix<N>& identity();
  Matrix<N>& translate(float tx, float ty, float tz);
  Matrix<N>& rotate(const Vector3f &axis, float angle);
  Matrix<N>& scale(float sx, float sy, float sz);

  //template
  friend std::ostream& operator<<(std::ostream &os, const Matrix<N> &mat)
  {
    //const int N = 4;
    //LOGD("PRINTING...");

    
    os << "Matrix<" << N << "x" << N << ">  -->\n";
    os << std::setprecision(2) << std::fixed;
    for(int r = 0; r < N; r++)
      {
	os << "  ";
	for(int c = 0; c < N; c++)
	  { os << mat(r, c) << (c == N - 1 ? "" : "  "); }
	os << "\n";
      }
    
    return os;
  }
};


template<int N>
Matrix<N>::Matrix(const QMatrix4x4 &other)
  : Matrix()
{
  if(N != 4)
    { throw std::exception(); }
  for(int i = 0; i < 4; i++)
    {
      for(int j = 0; j < 4; j++)
	{
	  at(i, j) = other(i, j);
	}
    }
}

template<int N>
Matrix<N>::Matrix(const float *pData)
//  : mData(new float[N*N])
{
  mData = new float[N*N];
  if(pData)
    { 
      //LOGD("CREATING MATRIX4 (DATA)...");
      std::copy(pData, pData + elemSize, mData);
    }
  else
    { 
      //LOGD("CREATING MATRIX4 (NO DATA)...");
      std::memset(mData, 0, dataSize);
    }
}

template<int N>
Matrix<N>::Matrix(const Matrix<N> &other)
  : Matrix(other.mData)
{
  //LOGD("CREATING MATRIX$ (OTHER MAT)...");
}
/*
template<int N>
Matrix<N>::~Matrix()
{
  if(mData)
    {
      LOGD("FREEING MATRIX4...");
      delete [] mData;
    }
  else
    {
      LOGD("MATRIX4 NO FREE...");
    }
    }*/

template<int N>
Matrix<N>& Matrix<N>::operator=(const Matrix<N> &other)
{
  //LOGD("MATRIX4 ASSIGNMENT...");
  std::copy(other.mData, other.mData + elemSize, mData);
  return *this;
}

//template<int N>
//unsigned int Matrix<N>::calcIndex(int row, int col) const
//{ return row*N + col; }

template<int N>
Matrix<N> Matrix<N>::transposed() const
{
  Matrix<N> result;
  for(int r = 0; r < N; r++)
    {
      for(int c = 0; c < N; c++)
	{ result(c, r) = mData[r*N + c]; }
    }
  return result;
}

// data access
template<int N>
float& Matrix<N>::operator()(unsigned int r, unsigned int c)
{ return mData[r*N+c]; }
template<int N>
float Matrix<N>::operator()(unsigned int r, unsigned int c) const
{ return mData[r*N+c]; }

template<int N>
const float* Matrix<N>::data() const
{ return mData; }
template<int N>
float* Matrix<N>::dataCopy() const
{
  float *copy = new float[N*N];
  std::copy(mData, mData + elemSize, copy);
  return copy;
}



// operations
template<int N>
Matrix<N>& Matrix<N>::operator*=(const Matrix<N> &other)
{
  Matrix<N> orig(*this);
  
  for(int r = 0; r < N; r++)
    {
      for(int c = 0; c < N; c++)
	{
	  at(r,c) = 0.0;
	  for(int p = 0; p < N; p++)
	    { at(r,c) += orig(r, p) * other(p, c); }
	}
    }
  return *this;
}
template<int N>
Matrix<N> Matrix<N>::operator*(const Matrix<N> &other) const
{
  Matrix<N> result;
  for(int r = 0; r < N; r++)
    {
      for(int c = 0; c < N; c++)
	{
	  result(r, c) = 0.0;
	  for(int p = 0; p < N; p++)
	    { result(r, c) += at(r,p) * other(p, c); }
	}
    }
  return result;
}
template<int N>
Matrix<N>& Matrix<N>::operator+=(const Matrix<N> &other)
{
  for(int r = 0; r < N; r++)
    {
      for(int c = 0; c < N; c++)
	{ at(r,c) += other(r, c); }
    }
    
  return *this;
}
template<int N>
Matrix<N> Matrix<N>::operator+(const Matrix<N> &other) const
{
  Matrix<N> result(*this);
  return result += other;
}
template<int N>
Matrix<N>& Matrix<N>::operator-=(const Matrix<N> &other)
{
  for(int r = 0; r < N; r++)
    {
      for(int c = 0; c < N; c++)
	{ mData[r*N+c] -= other(r, c); }
    }
  return *this;
}
template<int N>
Matrix<N> Matrix<N>::operator-(const Matrix<N> &other) const
{
  Matrix<N> result(*this);
  return result -= other;
}

/*
template<int N>
Vector<float, N> Matrix<N>::operator*(const Vector<float, N> &v) const
{
  Vector<float, N> result;
  for(int i = 0; i < N; i++)
    { // vector elements
      for(int j = 0; j < N; j++)
	{ // matrix columns
	  result[i] += v[i] * at(i, j);
	}
    }
  return result;
}

template<int N>
Vector<float, N> operator*(const Vector<float, N> &v, const Matrix<N> &m)
{
  Vector<float, N> result;
  for(int i = 0; i < N; i++)
    { // vector elements
      for(int j = 0; j < N; j++)
	{ // matrix columns
	  result[i] += v[i] * m(i, j);
	}
    }
  return result;
}
*/


template<int N>
Matrix<N>& Matrix<N>::operator*=(const float &scalar)
{
  for(int r = 0; r < N; r++)
    {
      for(int c = 0; c < N; c++)
	{ mData[r*N+c] *= scalar; }
    }
  return *this;
}
template<int N>
Matrix<N> Matrix<N>::operator*(const float &scalar) const
{
  Matrix<N> result;
  for(int r = 0; r < N; r++)
    {
      for(int c = 0; c < N; c++)
	{ result(r, c) = mData[r*N+c]*scalar; }
    }
  return result;
}
//template<int N>
///Matrix<N> operator*(const float &scalar, const Matrix<N> &mat) const
//{
//  return mat * scalar;
//}
template<int N>
Matrix<N>& Matrix<N>::operator/=(const float &scalar)
{
  for(int r = 0; r < N; r++)
    {
      for(int c = 0; c < N; c++)
	{ mData[r*N+c] /= scalar; }
    }
  return *this;
}
template<int N>
Matrix<N> Matrix<N>::operator/(const float &scalar) const
{
  Matrix<N> result(*this);
  return result /= scalar;
}


template<int N>
Vector<float, N> operator*(const Matrix<N> &m, const Vector<float, N> &v)
{
  Vector<float, N> result;
  for(int i = 0; i < N; i++)
    { // vector elements
      for(int j = 0; j < N; j++)
	{ // matrix columns
	  result[i] += v[i] * m(i, j);
	}
    }
  return result;
}

template<int N>
Vector<float, N> operator*(const Vector<float, N> &v, const Matrix<N> &m)
{
  Vector<float, N> result;
  for(int i = 0; i < N; i++)
    { // vector elements
      for(int j = 0; j < N; j++)
	{ // matrix columns
	  result[j] += v[i] * m(j, i);
	}
    }
  return result;
}


template<int N>
Vector<float, N> Matrix<N>::operator*(const Vector<float, N> &v) const
{
  return Vector<float, N>({at(0,0)*v[0] + at(0,1)*v[1] + at(0,2)*v[2] + at(0,3)*v[3],
                           at(1,0)*v[0] + at(1,1)*v[1] + at(1,2)*v[2] + at(1,3)*v[3],
                           at(2,0)*v[0] + at(2,1)*v[1] + at(2,2)*v[2] + at(2,3)*v[3],
                           at(3,0)*v[0] + at(3,1)*v[1] + at(3,2)*v[2] + at(3,3)*v[3] });
}

template<int N>
Matrix<N>& Matrix<N>::identity()
{
  std::memset(mData, 0, dataSize);
  for(int i = 0; i < N; i++)
    { at(i,i) = 1; }
  return *this;
}
template<int N>
Matrix<N>& Matrix<N>::translate(float tx, float ty, float tz)
{
  if(N != 4) throw std::exception();
  identity();
  at(0,3) = tx;
  at(1,3) = ty;
  at(2,3) = tz;
  return *this;
}

template<int N>
Matrix<N>& Matrix<N>::rotate(const Vector3f &axis, float angle)
{
  if(N != 4) throw std::exception();
  identity();
  at(0, 0) = (1*axis[0] + cosf(angle)*(axis[1] + axis[2]));
  at(0, 1) = -sinf(angle)*axis[2];
  at(0, 2) = sinf(angle)*axis[1];
  at(1, 0) = sinf(angle)*axis[2];
  at(1, 1) = (cosf(angle)*(axis[0] + axis[2]) + 1*axis[1]);
  at(1, 2) = -sinf(angle)*axis[0];
  at(2, 0) = -sinf(angle)*axis[1];
  at(2, 1) = sinf(angle)*axis[0];
  at(2, 2) = (cosf(angle)*(axis[0] + axis[1]) + 1*axis[2]);
  at(3, 3) = 1;
  return *this;
}

template<int N>
Matrix<N>& Matrix<N>::scale(float sx, float sy, float sz)
{
  if(N != 4) throw std::exception();
  at(0,0) = sx;
  at(1,1) = sy;
  at(2,2) = sz;
  at(3,3) = 1;
  return *this;
}

typedef Matrix<4> Matrix4;

Matrix4 matIdentity();
Matrix4 matTranslate(float tx, float ty, float tz);
Matrix4 matRotate(const Vector<float, 3> &axis, float angle);
Matrix4 matScale(float sx, float sy, float sz);
Matrix4 matView(const Vector<float, 3> &center,
		   const Vector<float, 3> &eye,
		   const Vector<float, 3> &up );
Matrix4 matProjection(float fov, float aspect, float near, float far);


#endif // GEOMETRY_HPP
