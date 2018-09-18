#include "player.hpp"
#include "logging.hpp"
#include "shader.hpp"
#include "geometry.hpp"
#include "params.hpp"
#include "fluid.hpp"


// pos should be the BOTTOM-CENTER of the player's bounding box.
Camera::Camera(Point3f pos, Vector3f forward, Vector3f up, Vector3f eyeOffset)
  : cCollidable(pos + Vector3f{0,0,PLAYER_SIZE[2]*0.5f}, PLAYER_SIZE),
    mEyeOffset(eyeOffset)
{
  mFrustum.setView(forward, up);
  mFrustum.setPos(mBox.center() + eyeOffset);
}

void Camera::setProjection(float fov, float aspect, float zNear, float zFar)
{
  mFrustum.setProjection(fov, aspect, zNear, zFar);
}

Frustum* Camera::getFrustum()
{ return &mFrustum; }

void Camera::setPos(const Point3f &newPos)
{
  mBox.setCenter(newPos);
  mFrustum.setPos(mBox.center() + mEyeOffset);
}

void Camera::rotate(float pitch, float yaw)
{
  mFrustum.rotate(pitch, yaw);
}

Vector3f Camera::getEye() const
{
  return mFrustum.getEye();
}
Point3f Camera::getPos() const
{
  return mBox.center();
}

Matrix4 Camera::getView() const
{
  return mFrustum.getView();
}
Matrix4 Camera::getProjection() const
{
  return mFrustum.getProjection();
}






Player::Player(Point3f pos, Vector3f forward, Vector3f up, World *world,
               Vector3f eyeOffset, float reach )
  : Camera(pos, forward, up, eyeOffset), mWorld(world), mReach(reach),
    mHighlightModel("./res/highlight.obj")
{ }

Player::~Player()
{
  
}


void Player::setSelectMode(bool mode)
{
  mSelectMode = mode;
}
void Player::select(const Point2f &pos, const Matrix4 &proj)
{
  mSelectPos = pos;
  mSelectPos[1] *= -1.0f;
  if(mSelectMode)
    { // find vector mouse is hovering over
      Matrix<4> pInv(proj.getQMat().inverted());
      Matrix<4> vInv(getView().getQMat().inverted());
      Vector<float, 4> pos({mSelectPos[0], mSelectPos[1], -1.0f, 1.0f});
      // inverse projection
      Vector<float, 4> rayEye = pInv * pos;
      rayEye[2] = -1.0f;
      rayEye[3] = 0.0f;
      // inverse view
      Vector<float, 4> rayWorld = vInv * rayEye;
      mSelectRay = Vector3f({rayWorld[0], rayWorld[1], rayWorld[2]}).normalized();
    }
}

CompleteBlock Player::selectedBlock()
{
  return mSelectedBlock;
}

bool Player::pickUp()
{
  if(mSelectedBlock.type != block_t::NONE)
    {
      LOGI("Player picked up block! --> %d", (int)mSelectedBlock.type);
      onPickup(mSelectedBlock.type);
      mWorld->setBlock(mSelectedPos, block_t::NONE);
    }
  
  return true;
}

void Player::place()
{
  CompleteBlock block = onPlace();
  if(block.type != block_t::NONE)
    {
      if(mSelectedBlock.type != block_t::NONE)
	{
	  if(!mBox.collidesEdge(cBoundingBox(mSelectedPos + mSelectedFace + Vector3f{0.5, 0.5, 0.5},
                                             Vector3f{1, 1, 1} )))
	    {
	      LOGI("Player placed block! --> %d", (int)mSelectedBlock.type);
	      mWorld->setBlock(mSelectedPos + mSelectedFace, block.type, block.data);
	    }
	}
    }
}
bool Player::initGL(QObject *qParent)
{
  LOGI("Initializing player GL...");
  LOGI("  Creating block highlight shader...");
  mWireShader = new Shader(qParent);
  if(!mWireShader->loadProgram("./shaders/wireframe.vsh", "./shaders/wireframe.fsh",
			       {"posAttr", "normalAttr", "texCoordAttr"}, {"pvm"}))
    {
      LOGE("  Block highlight shader failed to load!");
      return false;
    }
  LOGI("  Initializing block highlight model...");
  mWireShader->bind();
  mHighlightModel.initGL(mWireShader);
  mWireShader->release();
  LOGI("Player GL initialized.");
  return true;
}
void Player::cleanupGL()
{
  LOGI("Uninitializing player GL...");
  LOGI("  Uninitializing block highlight model...");
  mHighlightModel.cleanupGL();
  LOGI("  Deleting block highlight shader...");
  delete mWireShader;
  LOGI("Player GL uninitialized.");
}

void Player::render(Matrix4 pvm)
{
  Vector3f selectRay = (mSelectMode ? mSelectRay : getEye());
  if(!mWorld->rayCast(mBox.center() + mEyeOffset, selectRay, mReach,
		      mSelectedBlock, mSelectedPos, mSelectedFace ))
    { mSelectedBlock = {block_t::NONE, nullptr}; }
  
  if(mSelectedBlock.type != block_t::NONE)
    {
      mWireShader->bind();
      mWireShader->setUniform("pvm", pvm);
      mHighlightModel.render(mWireShader,
                             pvm*matTranslate(mSelectedPos[0], mSelectedPos[1], mSelectedPos[2]));
      mWireShader->release();
    }
}

void Player::addForce(float right, float forward)
{
  mMoveForce += Vector3f{right, forward, 0.0};
}
void Player::addXForce(float right)
{
  mMoveForce[0] = right;
}
void Player::addYForce(float forward)
{
  mMoveForce[1] = forward;
}
void Player::addZForce(float up)
{
  mMoveForce[2] = up;
}
Point3i Player::getCollisions() const
{
  return mGrounded;
}



void Player::update(double dt)
{
  onUpdate(dt);
  Vector3f forward = mFrustum.getForward();
  Vector3f right = mFrustum.getRight();
  Vector3f up = mFrustum.getUp();

  mAccel = Vector3f{right[0], right[1], 0.0}  * mMoveForce[0] + Vector3f{forward[0], forward[1], 0} * mMoveForce[1] + up * mMoveForce[2] - mVel*800.0f*dt;
  mVel += mAccel * dt;
  
  Vector3f dPos = mVel * dt;
  Point3f newMin = mBox.minPoint() + dPos;
  Point3f newMax = mBox.maxPoint() + dPos;
  Point3i newMini = Point3i{std::floor(newMin[0]),
			    std::floor(newMin[1]),
			    std::floor(newMin[2]) };
  Point3i newMaxi = Point3i{std::ceil(newMax[0])-1,
			    std::ceil(newMax[1])-1,
			    std::ceil(newMax[2])-1};
  
  Point3i min{std::floor(mBox.minPoint()[0]),
	      std::floor(mBox.minPoint()[1]),
	      std::floor(mBox.minPoint()[2])};
  Point3i max{std::ceil(mBox.maxPoint()[0])-1,
	      std::ceil(mBox.maxPoint()[1])-1,
	      std::ceil(mBox.maxPoint()[2])-1};

  for(int i = 0; i < 3; i++)
    {
      if(mIsGrounded[i] && ((dPos[i] <= 0) != (mGroundedDir[i] <= 0)))
        { // unground if direction reversed (away from ground)
          std::cout << (i==0 ? "X" : (i==1 ? "Y" : "Z"))
                    << " no longer grounded: " << mIsGrounded << " : "
                    << mGrounded << " : dPos: " << dPos
                   << ", gdir: " << mGroundedDir << "\n";
          mIsGrounded[i] = false;
          mGrounded[i] = 0;
        }
    }
  
  const Point3i before({ dPos[0] < 0 ? min[0] : (dPos[0] > 0 ? max[0] : 0),
                         dPos[1] < 0 ? min[1] : (dPos[1] > 0 ? max[1] : 0),
                         dPos[2] < 0 ? min[2] : (dPos[2] > 0 ? max[2] : 0) });
  mBox.move(dPos);
  min = Point3i{std::floor(mBox.minPoint()[0]),
		std::floor(mBox.minPoint()[1]),
		std::floor(mBox.minPoint()[2]) };
  max = Point3i{std::ceil(mBox.maxPoint()[0])-1,
		std::ceil(mBox.maxPoint()[1])-1,
		std::ceil(mBox.maxPoint()[2])-1};
  const Point3i after{ dPos[0] < 0 ? min[0] : (dPos[0] > 0 ? max[0] : 0),
                       dPos[1] < 0 ? min[1] : (dPos[1] > 0 ? max[1] : 0),
                       dPos[2] < 0 ? min[2] : (dPos[2] > 0 ? max[2] : 0) };
  
  Point3f center = mBox.center();
  Point3i vdir{mVel[0] < 0 ? -1 : 1,
	       mVel[1] < 0 ? -1 : 1,
	       mVel[2] < 0 ? -1 : 1 };
  
  for(int i = 0; i < 3; i++)
    {
      Point3i bPos = after;
      Point3i bMin = min;
      Point3i bMax = max;
      int i1 = (i+1)%3;
      int i2 = (i+2)%3;
      if(mIsGrounded[i])
        {
          bPos[i] = mGrounded[i];
          
	  if(mIsGrounded[i1])
	    {
              if(mGroundedDir[i1] < 0)
                { bMin[i1] = mGrounded[i1] + 1; }
              else
                { bMax[i1] = mGrounded[i1] - 1; }
            }
	  if(mIsGrounded[i2])
	    {
              if(mGroundedDir[i2] < 0)
                { bMin[i2] = mGrounded[i2] + 1; }
              else
                { bMax[i2] = mGrounded[i2] - 1; }
            }
          
	  bool hit = false;
	  for(bPos[i1] = bMin[i1]; bPos[i1] <= bMax[i1] && !hit; bPos[i1]++)
            for(bPos[i2] = bMin[i2]; bPos[i2] <= bMax[i2] && !hit; bPos[i2]++)
              {
                if(mWorld->getType(bPos) != block_t::NONE)
                  { // still grounded -- correct position
                    hit = true;
                    mVel[i] = 0.0;
                    center[i] = mGrounded[i] + ((mGroundedDir[i] < 0) ?
                                                (mBox.size()[i] / 2.0f + 1) :
                                                (-mBox.size()[i] / 2.0f));
                  }
              }
          
          if(!hit)
	    { // no longer grounded
              mIsGrounded[i] = false;
              mGrounded[i] = 0;
            }
        }
      else if(before[i] != after[i])
	{
	  if(mIsGrounded[i1])
	    {
              if(mGroundedDir[i1] < 0)
                { bMin[i1] = mGrounded[i1] + 1; }
              else
                { bMax[i1] = mGrounded[i1] - 1; }
            }
	  if(mIsGrounded[i2])
	    {
              if(mGroundedDir[i2] < 0)
                { bMin[i2] = mGrounded[i2] + 1; }
              else
                { bMax[i2] = mGrounded[i2] - 1; }
            }

	  bool hit = false;
	  for(bPos[i1] = bMin[i1]; bPos[i1] <= bMax[i1] && !hit; bPos[i1]++)
            for(bPos[i2] = bMin[i2]; bPos[i2] <= bMax[i2] && !hit; bPos[i2]++)
              {
                if(mWorld->getType(bPos) != block_t::NONE)
                  { // direction is grounded
                    hit = true;
                    mIsGrounded[i] = true;
                    mGrounded[i] = bPos[i];
                    mGroundedDir[i] = vdir[i];
                    mVel[i] = 0;
                    center[i] = bPos[i] + ((mGroundedDir[i] < 0) ?
                                           (mBox.size()[i] / 2.0f + 1) :
                                           (-mBox.size()[i] / 2.0f));
                    break;
                  }
              }
	}
    }
  mBox.setCenter(center);

  Point3i chunkPos = World::chunkPos(Point3i{(int)center[0],(int)center[1],(int)center[2]});
  static Point3i lastChunkPos = chunkPos;
  if(chunkPos != lastChunkPos)
    {
      mWorld->setCenter(chunkPos);
      lastChunkPos = chunkPos;
    }
  
  mVel[0] *= PLAYER_DRAG;
  mVel[1] *= PLAYER_DRAG;
  setPos(mBox.center());
}



GodPlayer::GodPlayer(Point3f pos, Vector3f forward, Vector3f up, World *world)
  : Player(pos, forward, up, world,
           Vector3f{0,0,PLAYER_EYE_HEIGHT}, 256.0f) // far reach
{ }

void GodPlayer::onUpdate(double dt)
{ }

void GodPlayer::setPlaceBlock(block_t type, BlockData *data)
{
  mPlaceBlock = {type, data ? data->copy() : nullptr};
}
void GodPlayer::nextPlaceBlock()
{
  mPlaceBlock.type = (block_t)(((int)mPlaceBlock.type + 1) % (int)simple_t::END);
}
void GodPlayer::prevPlaceBlock()
{
  mPlaceBlock.type = (block_t)(((int)mPlaceBlock.type - 1 + (int)simple_t::END) % (int)simple_t::END);
}

void GodPlayer::onPickup(block_t type)
{ /* do nothing 'cause you're a god */ }
CompleteBlock GodPlayer::onPlace()
{ return mPlaceBlock; }

FpsPlayer::FpsPlayer(Point3f pos, Vector3f forward, Vector3f up, World *world)
  : Player(pos, forward, up, world,
           Vector3f{0, 0, PLAYER_EYE_HEIGHT}, 4.0f) // short reach
{
  
}


void FpsPlayer::jump(float strength)
{
  if(mIsGrounded[2])
    {
      mVel[2] = strength*GRAVITY / 80;
      mIsGrounded[2] = false;
      mGrounded[2] = 0;
    }
}


void FpsPlayer::onUpdate(double dt)
{ // add force of gravity
  mMoveForce[2] = -GRAVITY*dt;
}

void FpsPlayer::onPickup(block_t type)
{
  mInventory.push(type);
}
CompleteBlock FpsPlayer::onPlace()
{
  block_t b = mInventory.front();
  mInventory.pop();
  mNextBlock = b;
  return {mNextBlock, nullptr};
}
