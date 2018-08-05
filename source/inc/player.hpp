#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <QMatrix4x4>

#include "block.hpp"
#include "geometry.hpp"
#include "collision.hpp"
#include "model.hpp"
#include "world.hpp"


class cCamera : public cCollidable
{
public:
  cCamera(Vector3f pos, Vector3f forward, Vector3f up, Vector3f eyeOffset);

  void rotate(float pitch, float yaw);
  Matrix4 getView();

  void setPos(const Point3f &newPos);

  Vector3f getEye() const;
  Point3f getPos() const;

protected:
  const Vector3f mEyeOffset;
  
  Matrix4 mViewMat;
  Vector3f mForward;
  Vector3f mUp;
  Vector3f mRight;
  int mLookingUp = 0;
  
  float mPitch = 0.0;
  float mPitchMult = 1.0;
  float mYawMult = 1.0;
  float mMoveSpeed = 1.0;


};



// adds functionality for picking and placing blocks.
class cShader;
class cPlayer : public cCamera
{
public:
  cPlayer(QObject *qparent, Vector3f pos, Vector3f forward, Vector3f up, cWorld *world, Vector3f eyeOffset, float reach);
  ~cPlayer();
  
  bool pickUp();
  void place();

  void addForce(float right, float forward);
  void addXForce(float right);
  void addYForce(float forward);
  void addZForce(float up);
  
  virtual void update(double dt);
  virtual void onUpdate(double dt) {};
  Point3i getCollisions() const;
  
  virtual bool initGL();
  virtual void cleanupGL();
  virtual void render(Matrix4 pvm);
  
protected:
  std::vector<block_t> mInventory;

  cWorld *mWorld;
  float mReach;

  cShader *mWireShader = nullptr;
  cModelObj mHighlightModel;
  //cBlock *mSelected = nullptr;
  block_t mSelectedType = block_t::NONE;
  Point3i mSelectedPos;
  Vector3i mSelectedFace;
   
  Vector3f mVel;
  Vector3f mAccel;
  Vector3f mMoveForce;
  Vector3i mGrounded = Vector3i{0,0,0};
  Vector3i mGroundedDir = Vector3i{0,0,0};
  Vector<bool, 3> mIsGrounded = Vector<bool,3>{false, false, false};
};




class cGodPlayer : public cPlayer
{
public:
  cGodPlayer(QObject *qparent, Vector3f pos, Vector3f forward, Vector3f up, cWorld *world);

  virtual void onUpdate(double dt) override;
  
private:
  
};






class cFpsPlayer : public cPlayer
{
public:
  cFpsPlayer(QObject *qparent, Vector3f pos, Vector3f forward, Vector3f up, cWorld *world);

  void jump(float strength);

  virtual void onUpdate(double dt) override;

private:

  
};

#endif // PLAYER_HPP
