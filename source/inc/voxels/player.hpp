#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <QMatrix4x4>

#include "block.hpp"
#include "geometry.hpp"
#include "collision.hpp"
#include "model.hpp"
#include "world.hpp"
#include "frustum.hpp"

#include <queue>

class Shader;
class QObject;

class Camera : public cCollidable
{
public:
  Camera(Point3f pos, Vector3f forward, Vector3f up, Vector3f eyeOffset);

  void rotate(float pitch, float yaw);
  Matrix4 getView() const;
  Matrix4 getProjection() const;

  void setProjection(float fov, float aspect, float zNear, float zFar);
  Frustum* getFrustum();
  
  void setPos(const Point3f &newPos);

  Vector3f getEye() const;
  Point3f getPos() const;

protected:
  const Vector3f mEyeOffset;

  Frustum mFrustum;
};



// adds functionality for picking and placing blocks.
class Player : public Camera
{
public:
  Player(Point3f pos, Vector3f forward, Vector3f up, World *world,
         Vector3f eyeOffset, float reach );
  ~Player();
  
  void setSelectMode(bool mode);
  void select(const Point2f &pos, const Matrix4 &proj);
  CompleteBlock selectedBlock();
  //BlockData* selectedBlockData();
  
  bool pickUp();
  void place();

  void addForce(float right, float forward);
  void addXForce(float right);
  void addYForce(float forward);
  void addZForce(float up);
  
  virtual void update(double dt);
  Point3i getCollisions() const;
  
  virtual bool initGL(QObject *qParent);
  virtual void cleanupGL();
  virtual void render(Matrix4 pvm);
  
protected:
  World *mWorld;
  float mReach;
  
  bool mSelectMode = false; // whether to select with free mouse instead of rotating
  Point2f mSelectPos; // screen-space coordinates ([-1, 1])
  Vector3f mSelectRay;

  Shader *mWireShader = nullptr;
  ModelObj mHighlightModel;
  CompleteBlock mSelectedBlock = {block_t::NONE, nullptr};
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
  GodPlayer(Point3f pos, Vector3f forward, Vector3f up, World *world);

  void setPlaceBlock(block_t type, BlockData *data);
  void nextPlaceBlock();
  void prevPlaceBlock();
  
protected:
  CompleteBlock mPlaceBlock = {block_t::NONE, nullptr};
  
  virtual void onUpdate(double dt) override;
  virtual void onPickup(block_t type) override;
  virtual CompleteBlock onPlace() override;
};

class FpsPlayer : public Player
{
public:
  FpsPlayer(Point3f pos, Vector3f forward, Vector3f up, World *world);

  void jump(float strength);

protected:
  std::queue<block_t> mInventory;
  block_t mNextBlock = block_t::NONE;

  virtual void onUpdate(double dt) override;
  virtual void onPickup(block_t type) override;
  virtual CompleteBlock onPlace() override;  
};

#endif // PLAYER_HPP
