#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <QMatrix4x4>

#include "block.hpp"
#include "geometry.hpp"
#include "collision.hpp"
#include "model.hpp"
#include "world.hpp"

#include <queue>


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
  
  void setSelectMode(bool mode);
  void select(const Point2f &pos, const Matrix4 &proj);

  cBlock* selectedBlock();
  
  bool pickUp();
  void place();

  void addForce(float right, float forward);
  void addXForce(float right);
  void addYForce(float forward);
  void addZForce(float up);
  
  virtual void update(double dt);
  Point3i getCollisions() const;
  
  virtual bool initGL();
  virtual void cleanupGL();
  virtual void render(Matrix4 pvm);
  
protected:
  cWorld *mWorld;
  float mReach;
  
  bool mSelectMode = false; // whether to select with free mouse instead of rotating
  Point2f mSelectPos; // [-1, 1]
  Vector3f mSelectRay;

  cShader *mWireShader = nullptr;
  cModelObj mHighlightModel;
  cBlock *mSelectedBlock = nullptr;
  block_t mSelectedType = block_t::NONE;
  Point3i mSelectedPos;
  Vector3i mSelectedFace;
   
  Vector3f mVel;
  Vector3f mAccel;
  Vector3f mMoveForce;
  Vector3i mGrounded = Vector3i{0,0,0};
  Vector3i mGroundedDir = Vector3i{0,0,0};
  Vector<bool, 3> mIsGrounded = Vector<bool,3>{false, false, false};

  virtual void onUpdate(double dt) {}
  virtual void onPickup(block_t type) {}
  virtual block_t onPlace() { return block_t::NONE; }
  
};




class cGodPlayer : public cPlayer
{
public:
  cGodPlayer(QObject *qparent, Vector3f pos, Vector3f forward, Vector3f up, cWorld *world);

  void setPlaceBlock(block_t type);
  void nextPlaceBlock();
  void prevPlaceBlock();
  
protected:
  block_t mPlaceBlock = block_t::NONE;
  
  virtual void onUpdate(double dt) override;
  virtual void onPickup(block_t type) override;
  virtual block_t onPlace() override;
};






class cFpsPlayer : public cPlayer
{
public:
  cFpsPlayer(QObject *qparent, Vector3f pos, Vector3f forward, Vector3f up, cWorld *world);

  void jump(float strength);


protected:
  std::queue<block_t> mInventory;

  virtual void onUpdate(double dt) override;
  virtual void onPickup(block_t type) override;
  virtual block_t onPlace() override;

  
};

#endif // PLAYER_HPP
