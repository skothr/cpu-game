#include "geometry.hpp"

Matrix<4> matIdentity()
{
  Matrix<4> mat;
  for(int i = 0; i < 4; i++)
    { mat(i,i) = 1; }
  return mat;
}
Matrix<4> matTranslate(float tx, float ty, float tz)
{
  Matrix<4> mat = matIdentity();
  mat(0,3) = tx;
  mat(1,3) = ty;
  mat(2,3) = tz;
  return mat;
}
Matrix<4> matRotate(const Vector<float, 3> &axis, float angle)
{
  Matrix<4> mat;
  mat(0,0)=(1*axis[0] + cosf(angle)*(axis[1] + axis[2]));
  mat(0,1)=-sinf(angle)*axis[2];
  mat(0,2)=sinf(angle)*axis[1];
  mat(1,0)=sinf(angle)*axis[2];
  mat(1, 1) = (cosf(angle)*(axis[0] + axis[2]) + 1*axis[1]);
  mat(1, 2) = -sinf(angle)*axis[0];
  mat(2,0)=-sinf(angle)*axis[1];
  mat(2,1)=sinf(angle)*axis[0];
  mat(2,2)=(cosf(angle)*(axis[0] + axis[1]) + 1*axis[2]);
  mat(3,3) = 1;
  return mat;
}
Matrix<4> matScale(float sx, float sy, float sz)
{
  Matrix<4> mat = matIdentity();
  mat(0,0) = sx;
  mat(1,1) = sy;
  mat(2,2) = sz;
  mat(3,3) = 1;
  return mat;
}
Matrix<4> matView(const Vector<float, 3> &center,
		   const Vector<float, 3> &eye,
		   const Vector<float, 3> &up )
{
  Matrix4 mat;
  auto right = crossProduct(up, eye);
  
  mat(0,0) = right[0]; mat(0,1) = right[1];   mat(0,2) = right[2];  mat(0,3) = center[0];
  mat(1,0) = up[0];    mat(1,1) = up[1];      mat(1,2) = up[2];     mat(1,3) = center[1];
  mat(2,0) = eye[0];   mat(2,1) = eye[1];     mat(2,2) = eye[2];    mat(2,3) = center[2];
  mat(3,0) = 0; mat(3,1) = 0; mat(3,2) = 0; mat(3,3)=1;

  mat = mat.transposed();
  
  return mat;
}
Matrix<4> matProjection(float fov, float aspect, float near, float far)
{
  
  float t = tanf(fov/2)*near;
  float b = -t;
  float r = t * aspect;
  float l = -t * aspect;
  
  Matrix<4> mat;
  mat(0,0) = 2 * near / (r - l);
  mat(1,1) = 2 * near / (t - b);
  mat(2,0) = (r+l)/(r-l);
  mat(2,1) = (t+b)/(t-b);
  mat(2,2) = -(far+near)/(far - near);
  mat(2,3) = -1;
  mat(3,2) = -2 * far * near /(far - near);
  
  /*
  float f = 1 / tanf(fov/2);

  Matrix<4> mat;
  mat(0,0) = f / aspect;
  mat(1,1) = f;
  //mat(2,0) = ;
  //mat(2,1) = ;
  mat(2,2) = (far+near)/(near-far);
  mat(3,2) = 2 * far * near /(near-far);
  mat(3,2) = -1;
  mat(3,3) = 0;
  */
  std::cout << "PERSPECTIVE:\n" << mat;
  return mat;
}
