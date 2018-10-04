#include "player.hpp"
#include "logging.hpp"
#include "shader.hpp"
#include "params.hpp"
#include "fluid.hpp"

#define PLAYER_EYE_OFFSET Vector3f{0,0,PLAYER_EYE_HEIGHT}

Player::Player(Point3f pos, Vector3f forward, Vector3f vertical, World *world)
  : mWorld(world), mCamera(pos + PLAYER_EYE_OFFSET), mBox(pos, PLAYER_SIZE),
    mEyeOffset(PLAYER_EYE_OFFSET), mHighlightModel("./res/highlight.obj")
{
  mCamera.setView(forward, vertical);
  mToolParams.sphere.radius = 1;
}

Player::~Player()
{
  
}

void Player::setGodMode(bool on)
{
  mGodMode = on;
  mReach = (on ? GOD_REACH : PLAYER_REACH);
}
void Player::setPlaceBlock(block_t type, BlockData *data)
{
  mPlaceBlock = {type, data ? data->copy() : nullptr};
}
void Player::setTool(tool_t tool)
{
  mTool = tool;
}
void Player::setToolRad(int rad)
{
  mToolParams.sphere.radius = rad;
}
void Player::nextPlaceBlock()
{
  mPlaceBlock.type = (block_t)(((int)mPlaceBlock.type + 1) % (int)simple_t::END);
}
void Player::prevPlaceBlock()
{
  mPlaceBlock.type = (block_t)(((int)mPlaceBlock.type - 1 + (int)simple_t::END) % (int)simple_t::END);
}

void Player::setPos(const Point3f &pos)
{
  mCamera.setPos(pos + mEyeOffset);
  mBox.setPos(pos - Vector3f{mBox.size()[0]/2.0f, mBox.size()[1]/2.0f, 0.0f});
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
      Matrix<4> vInv(mCamera.getView().getQMat().inverted());
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
      Point3i sPos = mSelectedPos;// + mSelectedFace;
      if(mPlaceBlock.type == block_t::NONE ||
         isSimpleBlock(mPlaceBlock.type) ||
         isFluidBlock(mPlaceBlock.type) )
        {
          std::cout << "Player picked up block (" << (int)mPlaceBlock.type << ") at: " << sPos << "\n";
          std::cout << "  Radius: " << mToolParams.sphere.radius << "\n";
          switch(mTool)
            {
            case tool_t::SPHERE:
              mWorld->setSphere(sPos, mToolParams.sphere.radius, block_t::NONE);
              break;
            case tool_t::CUBE:
              if(mHaveCubePoint && mCubeRemove)
                {
                  mWorld->setRange(mCubeP1, sPos, block_t::NONE);
                  mHaveCubePoint = false;
                }
              else
                {
                  mCubeP1 = sPos;
                  mHaveCubePoint = true;
                  mCubeRemove = true;
                }
              break;
            case tool_t::LINE:
              mWorld->setRange(sPos, sPos - mSelectedFace*mToolParams.sphere.radius, block_t::NONE);
              break;
            }
        }
      else
        {
          mWorld->setBlock(sPos, mPlaceBlock.type, mPlaceBlock.data);
        }
    }
  
  // if(mSelectedBlock.type != block_t::NONE)
  //   {
  //     LOGI("Player picked up block! --> %d", (int)mSelectedBlock.type);
  //     mWorld->setBlock(mSelectedPos, block_t::NONE);
  //   }
  
  return true;
}

void Player::place()
{
  if(mPlaceBlock.type != block_t::NONE)
    {
      Point3i sPos = mSelectedPos + mSelectedFace;
      if(isSimpleBlock(mPlaceBlock.type) ||
         isFluidBlock(mPlaceBlock.type) )
        {
          std::cout << "Player placed block (" << (int)mPlaceBlock.type << ") at: " << sPos << "\n";
          std::cout << "  Radius: " << mToolParams.sphere.radius << "\n";
          switch(mTool)
            {
            case tool_t::SPHERE:
              mWorld->setSphere(sPos, mToolParams.sphere.radius, mPlaceBlock.type, mPlaceBlock.data);
              break;
            case tool_t::CUBE:
              if(mHaveCubePoint && !mCubeRemove)
                {
                  mWorld->setRange(mCubeP1, sPos, mPlaceBlock.type, mPlaceBlock.data);
                  mHaveCubePoint = false;
                }
              else
                {
                  mCubeP1 = sPos;
                  mHaveCubePoint = true;
                  mCubeRemove = false;
                }
                  
              break;
            case tool_t::LINE:
              {
                 mWorld->setRange(sPos, sPos + mSelectedFace*mToolParams.sphere.radius,
                                  mPlaceBlock.type, mPlaceBlock.data );
              }
              break;
            }
        }
      else
        {
          mWorld->setBlock(sPos, mPlaceBlock.type, mPlaceBlock.data);
        }
    }
}
bool Player::initGL(QObject *qParent)
{
  LOGI("Initializing player GL...");
  LOGI("  Creating block highlight shader...");
  mWireShader = new Shader(qParent);
  if(!mWireShader->loadProgram("./shaders/wireframe.vsh", "./shaders/wireframe.fsh",
			       {"posAttr", "normalAttr", "texCoordAttr"}, {"pvm", "stretch"}))
    {
      LOGE("  Block highlight shader failed to load!");
      return false;
    }
  LOGI("  Initializing block highlight model...");
  mWireShader->bind();
  mHighlightModel.initGL(mWireShader);
  mWireShader->setUniform("stretch", Vector3i{1,1,1});
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
  Vector3f selectRay = (mSelectMode ? mSelectRay : mCamera.getEye());
  if(!mWorld->rayCast(mCamera.getPos(), selectRay, (mGodMode ? GOD_REACH : PLAYER_REACH),
		      mSelectedBlock, mSelectedPos, mSelectedFace ))
    { mSelectedBlock = {block_t::NONE, nullptr}; }
  
  if(mSelectedBlock.type != block_t::NONE)
    {
      mWireShader->bind();
      mWireShader->setUniform("pvm", pvm);

      if(mHaveCubePoint)
        {
          Point3i sPos = mSelectedPos;
          if(!mCubeRemove)
            { sPos += mSelectedFace; }
          Point3f pos{std::min(sPos[0], mCubeP1[0]),
                      std::min(sPos[1], mCubeP1[1]),
                      std::min(sPos[2], mCubeP1[2])};
          Vector3f size{std::abs(sPos[0] - mCubeP1[0])+1.0f,
                        std::abs(sPos[1] - mCubeP1[1])+1.0f,
                        std::abs(sPos[2] - mCubeP1[2])+1.0f };
          mWireShader->setUniform("stretch", size);
          mHighlightModel.render(mWireShader,
                                 (pvm*matTranslate(pos[0], pos[1], pos[2])*
                                  matScale(size[0], size[1], size[2]) ));
        }
      else
        {
          mWireShader->setUniform("stretch", Vector3i{1,1,1});
          mHighlightModel.render(mWireShader,
                                 pvm*matTranslate(mSelectedPos[0],
                                                  mSelectedPos[1],
                                                  mSelectedPos[2]) );
        }
      mWireShader->release();
    }
}


void Player::moveX(int dir)
{
  mMoveForce[0] = PLAYER_SPEED * dir * (mSneaking ? 0.01f : (mRunning ? 10.0f : 1.0f));
}
void Player::moveY(int dir)
{
  mMoveForce[1] = PLAYER_SPEED * dir * (mSneaking ? 0.01f : (mRunning ? 10.0f : 1.0f));
}
void Player::moveZ(int dir)
{
  if(mGodMode)
    { mMoveForce[2] = PLAYER_SPEED * dir * (mSneaking ? 0.01f : (mRunning ? 10.0f : 1.0f)); }
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

void Player::sneak(bool sneaking)
{
  mSneaking = sneaking;
}
void Player::run(bool running)
{
  mRunning = running;
}
void Player::jump(bool start)
{
  if(mGodMode)
    { moveZ(start ? 1 : 0); }
  else if(start && mIsGrounded[2])
    {
      mVel[2] = GRAVITY / 320.0f;
      mIsGrounded[2] = false;
      mGrounded[2] = 0;
    }
}

#define PLAYER_DAMPING 800.0f

void Player::update(double dt)
{
  if(!mGodMode)
    { // apply gravity
      mMoveForce[2] = -GRAVITY*dt;
    }
  Vector3f forward = mCamera.getForward();
  Vector3f right = mCamera.getRight();
  Vector3f up = mCamera.getVertical();

  mAccel = (Vector3f{right[0], right[1], 0.0f} * mMoveForce[0] +
            Vector3f{forward[0], forward[1], 0.0f} * mMoveForce[1] +
            up * mMoveForce[2] - mVel*PLAYER_DAMPING*dt );
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
  
  Point3f center = mBox.pos() + mBox.size()/2.0f;
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
  center[2] -= mBox.size()[2]/2.0f;
  
  Point3i chunkPos = World::chunkPos(Point3i{(int)center[0], (int)center[1], (int)center[2]});
  static Point3i lastChunkPos = chunkPos;
  if(chunkPos != lastChunkPos)
    {
      mWorld->setCenter(chunkPos);
      lastChunkPos = chunkPos;
    }
  
  mVel[0] *= PLAYER_DRAG;
  mVel[1] *= PLAYER_DRAG;
  setPos(center);
}


/*
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
{ }// do nothing 'cause you're a god  }
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
*/
