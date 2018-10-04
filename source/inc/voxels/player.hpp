#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <QMatrix4x4>

#include "block.hpp"
#include "matrix.hpp"
#include "collision.hpp"
#include "model.hpp"
#include "world.hpp"
#include "camera.hpp"
#include "tool.hpp"

#include <queue>

class Shader;
class QObject;

#define PLAYER_REACH 4.0f
#define GOD_REACH 256.0f

// adds functionality for picking and placing blocks.
class Player
{
public:
  Player(Point3f pos, Vector3f forward, Vector3f up, World *world);
  ~Player();

  void setGodMode(bool on);
  void toggleGodMode() { mGodMode = !mGodMode; }
  void setPlaceBlock(block_t type, BlockData *data);
  void setTool(tool_t tool);
  void setToolRad(int rad);
  void nextPlaceBlock();
  void prevPlaceBlock();

  void setPos(const Point3f &pos);
  Point3f getPos() const { return mCamera.getPos() - mEyeOffset; }
  Camera* getCamera() { return &mCamera; }
  
  void setSelectMode(bool mode);
  void select(const Point2f &pos, const Matrix4 &proj);
  CompleteBlock selectedBlock();
  //BlockData* selectedBlockData();
  
  bool pickUp();
  void place();

  void moveX(int dir);
  void moveY(int dir);
  void moveZ(int dir);
  
  void addForce(float right, float forward);
  void addXForce(float right);
  void addYForce(float forward);
  void addZForce(float up);
  
  void jump(bool start);
  void sneak(bool sneaking);
  void run(bool running);
  
  virtual void update(double dt);
  Point3i getCollisions() const;
  
  virtual bool initGL(QObject *qParent);
  virtual void cleanupGL();
  virtual void render(Matrix4 pvm);
  
protected:
  World *mWorld;
  Camera mCamera;
  Vector3f mEyeOffset;
  cBoundingBox mBox;

  bool mGodMode = true;
  bool mSneaking = false;
  bool mRunning = false;

  float mReach = PLAYER_REACH;
  CompleteBlock mPlaceBlock = {block_t::NONE, nullptr};
  tool_t mTool = tool_t::SPHERE;
  ToolParams mToolParams;
  bool mSelectMode = false; // whether to select with free mouse instead of rotating
  Point2f mSelectPos; // screen-space coordinates ([-1, 1])
  Vector3f mSelectRay;

  Shader *mWireShader = nullptr;
  ModelObj mHighlightModel;
  CompleteBlock mSelectedBlock = {block_t::NONE, nullptr};
  Point3i mSelectedPos;
  Vector3i mSelectedFace;

  Point3i mCubeP1;
  bool mHaveCubePoint = false;
  bool mCubeRemove = false;
  
  Vector3f mVel;
  Vector3f mAccel;
  Vector3f mMoveForce;
  Vector3i mGrounded = Vector3i{0,0,0};
  Vector3i mGroundedDir = Vector3i{0,0,0};
  Vector<bool, 3> mIsGrounded = Vector<bool,3>{false, false, false};

  //virtual void onUpdate(double dt) {}
  //virtual void onPickup(block_t type) {}
  //virtual CompleteBlock onPlace() { return CompleteBlock{block_t::NONE, nullptr}; }
};


/*
class GodPlayer : public Player
{
public:
  GodPlayer(Point3f pos, Vector3f forward, Vector3f up, World *world);

  void setPlaceBlock(block_t type, BlockData *data);
  void nextPlaceBlock();
  void prevPlaceBlock();
  
protected:
  
  virtual void onUpdate(double dt) override;
  virtual void onPickup(block_t type) override;
  virtual CompleteBlock onPlace() override;
};

class FpsPlayer : public Player
{
public:
  FpsPlayer(Point3f pos, Vector3f forward, Vector3f up, World *world);

protected:
  std::queue<block_t> mInventory;
  block_t mNextBlock = block_t::NONE;

  virtual void onUpdate(double dt) override;
  virtual void onPickup(block_t type) override;
  virtual CompleteBlock onPlace() override;  
};
*/

#endif // PLAYER_HPP
