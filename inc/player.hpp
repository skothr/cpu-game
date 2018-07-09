#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <QMatrix4x4>

#include "block.hpp"
#include "geometry.hpp"
#include "collision.hpp"
#include "model.hpp"
#include "chunkMap.hpp"

struct PlayerDesc
{
  float touchRadius; //radius around which the player can interact
};


class cPlayer : public cCollidable
{
public:
  cPlayer(Vector3f pos, Vector3f forward, Vector3f up, cChunkMap *map);

  void addForce(float right, float forward);
  void setXForce(float right);
  void setYForce(float forward);
  void jump(float strength);
  void rotate(float pitch, float yaw);
  Matrix4 getView();
  
  bool interact();
  void place();

  void update(double dt);
  Vector3f getEye() const;

  void initGL(cShader *shader);
  void cleanupGL();
  void render(cShader *shader, Matrix4 pvm);
  
private:
  static const Vector3f mEyeOffset;
  
  Matrix4 mViewMat;
  Vector3f mForward;
  Vector3f mUp;
  Vector3f mRight;
  int mLookingUp = 0;
  
  Vector3f mVel;
  Vector3f mAccel;
  float mPitch = 0.0;

  Vector3f mMoveForce;
  Vector3i mGrounded = Vector3i{0,0,0};
  
  float mPitchMult = 1.0;
  float mYawMult = 1.0;
  float mMoveSpeed = 1.0;

  cChunkMap *mMap;
  std::vector<block_t> mInventory;

  cModelObj mHighlightModel;
  cBlock *mSelected = nullptr;
  Point3i mSelectedPos;
  Vector3i mSelectedFace;
};

#endif // PLAYER_HPP
