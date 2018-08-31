#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <QMatrix4x4>

#include "block.hpp"
#include "geometry.hpp"
#include "collision.hpp"
#include "model.hpp"
#include "world.hpp"

#include <queue>


class Camera : public cCollidable
{
public:
  Camera(Vector3f pos, Vector3f forward, Vector3f up, Vector3f eyeOffset);

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
class Shader;
class Player : public Camera
{
public:
  Player(QObject *qparent, Vector3f pos, Vector3f forward, Vector3f up, World *world, Vector3f eyeOffset, float reach);
  ~Player();
  
  void setSelectMode(bool mode);
  void select(const Point2f &pos, const Matrix4 &proj);

  CompleteBlock selectedBlock();
  
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
  World *mWorld;
  float mReach;
  
  bool mSelectMode = false; // whether to select with free mouse instead of rotating
  Point2f mSelectPos; // [-1, 1]
  Vector3f mSelectRay;

  Shader *mWireShader = nullptr;
  cModelObj mHighlightModel;
  CompleteBlock mSelectedBlock;
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
  virtual CompleteBlock onPlace() { return CompleteBlock{block_t::NONE, nullptr}; }
  
};




class GodPlayer : public Player
{
public:
  GodPlayer(QObject *qparent, Vector3f pos, Vector3f forward, Vector3f up, World *world);

  void setPlaceBlock(block_t type, BlockData *data);
  void nextPlaceBlock();
  void prevPlaceBlock();
  
protected:
  CompleteBlock mPlaceBlock;
  
  virtual void onUpdate(double dt) override;
  virtual void onPickup(block_t type) override;
  virtual CompleteBlock onPlace() override;
};






class FpsPlayer : public Player
{
public:
  FpsPlayer(QObject *qparent, Vector3f pos, Vector3f forward, Vector3f up, World *world);

  void jump(float strength);


protected:
  std::queue<block_t> mInventory;
  block_t mNextBlock;

  virtual void onUpdate(double dt) override;
  virtual void onPickup(block_t type) override;
  virtual CompleteBlock onPlace() override;

  
};

#endif // PLAYER_HPP
