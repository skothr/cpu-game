#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <algorithm>
#include <iostream>
#include <cstring>


template<typename T>
class Matrix
{
public:
  Matrix(int rows, int cols, const T *pData = nullptr);
  Matrix(const Matrix<T> &other);
  ~Matrix();
  
  Matrix<T>& operator=(const Matrix<T> &other);

  const int nRows;
  const int nCols;
  
private:
  T *mData = nullptr;

  unsigned int calcIndex(int row, int col) const;

public:
  // data access
  T& operator()(unsigned int r, unsigned int c);
  const T& operator()(unsigned int r, unsigned int c) const;
  T& at(unsigned int r, unsigned int c);
  const T& at(unsigned int r, unsigned int c) const;

  const T* data() const;
  unsigned int dataSize() const;

  // operations
  Matrix<T>& operator*=(const Matrix<T> &other);
  Matrix<T> operator*(const Matrix<T> &other) const;
  Matrix<T>& operator+=(const Matrix<T> &other);
  Matrix<T> operator+(const Matrix<T> &other) const;
  Matrix<T>& operator-=(const Matrix<T> &other);
  Matrix<T> operator-(const Matrix<T> &other) const;

  Matrix<T>& operator*=(const T &scalar);
  Matrix<T> operator*(const T &scalar) const;
  ///Matrix<T> operator*(const T &scalar, const Matrix<T> &mat) const;
  Matrix<T>& operator/=(const T &scalar);
  Matrix<T> operator/(const T &scalar) const;
};



template<typename T>
Matrix<T>::Matrix(int rows, int cols, const T *pData)
  : nRows(rows), nCols(cols)
{
  mData = new T[rows * cols];
  if(pData)
    { std::copy(pData, pData + dataSize(), mData); }
  else
    { std::memset(mData, 0, dataSize()); }
}

template<typename T>
Matrix<T>::Matrix(const Matrix<T> &other)
  : Matrix(other.nRows, other.nCols, other.data())
{ }

template<typename T>
Matrix<T>::~Matrix()
{
  delete[] mData;
  mData = nullptr;
}

template<typename T>
Matrix<T>& Matrix<T>::operator=(const Matrix<T> &other)
{
  if(nRows != other.nRows || nCols != other.nCols)
    {
      delete [] mData;
      mData = new T[nRows * nCols];
    std:copy(other.mData, other.mData + other.dataSize(), mData);
    }
}

template<typename T>
unsigned int Matrix<T>::calcIndex(int row, int col) const
{ return row*nRows + col; }


// data access
template<typename T>
T& Matrix<T>::operator()(unsigned int r, unsigned int c)
{ return mData[calcIndex(r, c)]; }
template<typename T>
const T& Matrix<T>::operator()(unsigned int r, unsigned int c) const
{ return mData[calcIndex(r, c)]; }
template<typename T>
T& Matrix<T>::at(unsigned int r, unsigned int c)
{ return mData[calcIndex(r, c)]; }
template<typename T>
const T& Matrix<T>::at(unsigned int r, unsigned int c) const
{ return mData[calcIndex(r, c)]; }

template<typename T>
const T* Matrix<T>::data() const
{ return mData; }
template<typename T>
unsigned int Matrix<T>::dataSize() const
{ return nRows * nCols * sizeof(T); }


// operations
template<typename T>
Matrix<T>& Matrix<T>::operator*=(const Matrix<T> &other)
{
  if(nRows != other.nRows || nCols != other.nCols)
    {
      std::cout << "ERROR: Matrix sizes cannot be multiplied --> (" << nRows << ", " << nCols << ") * (" << other.nRows << ", " << other.nCols << ")!\n";
      // TODO: exit cleanly
      exit(1);
    }
    
  for(int r = 0; r < nRows; r++)
    {
      for(int c = 0; c < nCols; c++)
	{
	  for(int p = 0; p < nCols; p++)
	    {
	      this->at(r, c) += this->at(r, p) * other(p, c);
	    }
	}
    }
  return *this;
}
template<typename T>
Matrix<T> Matrix<T>::operator*(const Matrix<T> &other) const
{
  if(nCols != other.rows)
    {
      std::cout << "ERROR: Matrix sizes cannot be multiplied --> (" << nRows << ", " << nCols << ") * (" << other.nRows << ", " << other.nCols << ")!\n";
      // TODO: exit cleanly
      exit(1);
    }
    
  Matrix<T> result(nCols, other.nRows);
  for(int r = 0; r < result.nRows; r++)
    {
      for(int c = 0; c < result.nCols; c++)
	{
	  for(int p = 0; p < nCols; p++)
	    {
	      result(r, c) += this->at(r, p) * other(p, c);
	    }
	}
    }
  return result;
}
template<typename T>
Matrix<T>& Matrix<T>::operator+=(const Matrix<T> &other)
{
  if(nRows != other.nRows || nCols != other.nCols)
    {
      std::cout << "ERROR: Matrix sizes cannot be summed --> (" << nRows << ", " << nCols << ") * (" << other.nRows << ", " << other.nCols << ")!\n";
      // TODO: exit cleanly
      exit(1);
    }
    
  for(int r = 0; r < nRows; r++)
    {
      for(int c = 0; c < nCols; c++)
	{
	  this->at(r, c)  += other(r, c);
	}
    }
    
  return *this;
}
template<typename T>
Matrix<T> Matrix<T>::operator+(const Matrix<T> &other) const
{
  if(nRows != other.nRows || nCols != other.nCols)
    {
      std::cout << "ERROR: Matrix sizes cannot be summed --> (" << nRows << ", " << nCols << ") * (" << other.nRows << ", " << other.nCols << ")!\n";
      // TODO: exit cleanly
      exit(1);
    }

  Matrix<T> result(*this);
    
  return result += other;
}
template<typename T>
Matrix<T>& Matrix<T>::operator-=(const Matrix<T> &other)
{
  if(nRows != other.nRows || nCols != other.nCols)
    {
      std::cout << "ERROR: Matrix sizes cannot be summed --> (" << nRows << ", " << nCols << ") * (" << other.nRows << ", " << other.nCols << ")!\n";
      // TODO: exit cleanly
      exit(1);
    }
    
  for(int r = 0; r < nRows; r++)
    {
      for(int c = 0; c < nCols; c++)
	{
	  this->at(r, c) -= other(r, c);
	}
    }
  return *this;
}
template<typename T>
Matrix<T> Matrix<T>::operator-(const Matrix<T> &other) const
{
  if(nRows != other.nRows || nCols != other.nCols)
    {
      std::cout << "ERROR: Matrix sizes cannot be summed --> (" << nRows << ", " << nCols << ") * (" << other.nRows << ", " << other.nCols << ")!\n";
      // TODO: exit cleanly
      exit(1);
    }

  Matrix<T> result(*this);
    
  return result -= other;
}


template<typename T>
Matrix<T>& Matrix<T>::operator*=(const T &scalar)
{
  for(int r = 0; r < nRows; r++)
    {
      for(int c = 0; c < nCols; c++)
	{
	  this->at(r, c) *= scalar;
	}
    }
  return *this;
}
template<typename T>
Matrix<T> Matrix<T>::operator*(const T &scalar) const
{
  Matrix<T> result(nRows, nCols);
  for(int r = 0; r < nRows; r++)
    {
      for(int c = 0; c < nCols; c++)
	{
	  result(r, c) = this->at(r, c)*scalar;
	}
    }
  return result;
}
//template<typename T>
///Matrix<T> operator*(const T &scalar, const Matrix<T> &mat) const
//{
//  return mat * scalar;
//}
template<typename T>
Matrix<T>& Matrix<T>::operator/=(const T &scalar)
{
  for(int r = 0; r < nRows; r++)
    {
      for(int c = 0; c < nCols; c++)
	{
	  this->at(r, c) /= scalar;
	}
    }
  return *this;
}
template<typename T>
Matrix<T> Matrix<T>::operator/(const T &scalar) const
{
  Matrix<T> result(*this);
  return result /= scalar;
}

#endif //MATRIX_HPP
