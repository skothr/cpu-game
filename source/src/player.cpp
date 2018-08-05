#include "player.hpp"
#include "logging.hpp"
#include "shader.hpp"

#define DRAG 0.98f

//static const Vector3f PLAYER_SIZE{2.0, 2.0, 8.0};
static const Vector3f PLAYER_SIZE{0.9, 0.9, 1.9};



// pos should be the BOTTOM-CENTER of the player's bounding box.
cCamera::cCamera(Vector3f pos, Vector3f forward, Vector3f up, Vector3f eyeOffset)
  : cCollidable(pos + Vector3f{0,0,PLAYER_SIZE[2]*0.5f}, PLAYER_SIZE),
    mForward(forward.normalized()), mUp(up.normalized()),
    mRight(crossProduct(forward, up).normalized()), mEyeOffset(eyeOffset)
{
  
}

void cCamera::setPos(const Point3f &newPos)
{
  mBox.setCenter(newPos + Vector3f{0,0,PLAYER_SIZE[2]*0.5f});
}

void cCamera::rotate(float pitch, float yaw)
{
  QMatrix4x4 rot;
  rot.rotate(yaw * mYawMult, toQt(mUp));
  mForward = (mForward.asVector()*Matrix4(rot).transposed()).normalized();
  mRight = (crossProduct(mForward, mUp)).normalized();

  mPitch += pitch/-100.0f;
  if(mPitch > 1.0f)
    { mPitch = 1.0f; }
  else if(mPitch < -1.0f)
    { mPitch = -1.0f; }
}

Vector3f cCamera::getEye() const
{
  return (mForward*(1.0 - std::abs(mPitch)) + mUp * (mPitch)).normalized();
}
Point3f cCamera::getPos() const
{
  return mBox.center();
}

Matrix4 cCamera::getView()
{
  //std::cout << "Vpos:     " << mBox.center() << "\n";
  //std::cout << "Vforward: " << mForward << "\n";
  //std::cout << "Vup     : " << mUp << "\n";
  QMatrix4x4 v;
  v.lookAt(toQt(mBox.center() + mEyeOffset), toQt(mBox.center() + mEyeOffset + getEye()), toQt((std::abs(mPitch) == 1.0f ? mForward*-mPitch : mUp)));
  
  /*
  //std::cout << "VIEW:\n";
  for(int r = 0; r < 4; r++)
  {
  for(int c = 0; c < 4; c++)
  {
  std::cout << v(r, c) << " ";
  }
  std::cout << "\n";
  }
  */

  //std::cout << "F: " << mForward << ", P: " << mBox.center() << ", U: " << mUp << "\n";
  //std::cout << "VIEW: " << Matrix4(v) << "\n\n";
  //Matrix4 m = Matrix4(v);
  //std::cout << v(0,0) << ", " << v(2, 1) << ", " << v(1, 2) << ", " << v(1, 3) << "\n";
  return Matrix4(v);//matView(mBox.center(), mForward, mUp);;
}






cPlayer::cPlayer(QObject *qparent, Vector3f pos, Vector3f forward, Vector3f up, cWorld *world,
                 Vector3f eyeOffset, float reach )
  : cCamera(pos, forward, up, eyeOffset), mWorld(world), mReach(reach),
    mHighlightModel("./res/highlight.obj")
{
  Point3f ppos = mBox.center();
  //mWorld->setCenter(Point3i{std::floor(ppos[0]), std::floor(ppos[1]), std::floor(ppos[2])} / 16);
  
  mWireShader = new cShader(qparent);
}

cPlayer::~cPlayer()
{
  delete mWireShader;
}

bool cPlayer::pickUp()
{
  if(mSelectedType != block_t::NONE)
    {
      LOGD("PLAYER GOT BLOCK! --> %d", (int)mSelectedType);
      mInventory.push_back(mSelectedType);
      mWorld->set(mSelectedPos, block_t::NONE);
      mSelectedType = block_t::NONE;
    }
  
  return true;
}

void cPlayer::place()
{
  if(mInventory.size() > 0)
    {
      if(mSelectedType != block_t::NONE)
	{
	  if(!mBox.collidesEdge(cBoundingBox(mSelectedPos + mSelectedFace + Vector3f{0.5, 0.5, 0.5}, Vector3f{1, 1, 1})))
	    {
	      LOGD("PLAYER PLACED BLOCK at: ");
	      std::cout << mSelectedPos << " (face " << mSelectedFace << ")\n";
	      if(mWorld->set(mSelectedPos + mSelectedFace, mInventory.back()))
		{
		  mInventory.pop_back();
		}
	    }
	}
    }
}
bool cPlayer::initGL()
{
  LOGD("PLAYER INIT GL");
  if(!mWireShader->loadProgram("./shaders/wireframe.vsh", "./shaders/wireframe.fsh",
			       {"posAttr", "normalAttr", "texCoordAttr"}, {"pvm"}))
    {
      LOGE("Wireframe shader failed to load!");
      return false;
    }
  mWireShader->bind();
  mHighlightModel.initGL(mWireShader);
  mWireShader->release();
  LOGD("PLAYER DONE INIT GL");
  return true;
}
void cPlayer::cleanupGL()
{
  mHighlightModel.cleanupGL();
}

void cPlayer::render(Matrix4 pvm)
{
  //LOGD("PLAYER RENDER");
  mWireShader->bind();
  mWireShader->setUniform("pvm", pvm);
  
  if(!mWorld->rayCast(mBox.center() + mEyeOffset, getEye(), mReach,
		      mSelectedType, mSelectedPos, mSelectedFace ))
    {
      mSelectedType = block_t::NONE;
    }
  if(mSelectedType != block_t::NONE)
    {
      //LOGI("Selected block --> (%d, %d, %d)", mSelectedPos[0], mSelectedPos[1], mSelectedPos[2]);
      mHighlightModel.render(mWireShader,
                             pvm*matTranslate(mSelectedPos[0], mSelectedPos[1], mSelectedPos[2]));
      //std::cout << "HIGHLIGHTED: " << mHighlighted << "\n";
    }
  mWireShader->release();
}


void cPlayer::addForce(float right, float forward)
{
  mMoveForce += Vector3f{right, forward, 0.0};
}
void cPlayer::addXForce(float right)
{
  mMoveForce[0] = right;
}
void cPlayer::addYForce(float forward)
{
  mMoveForce[1] = forward;
}
void cPlayer::addZForce(float up)
{
  mMoveForce[2] = up;
}
Point3i cPlayer::getCollisions() const
{
  return mGrounded;
}



void cPlayer::update(double dt)
{
  onUpdate(dt);
  
  Vector3f right = crossProduct(mForward, mUp).normalized();
  
  mAccel = Vector3f{right[0], right[1], 0.0}  * mMoveForce[0] + Vector3f{mForward[0], mForward[1], 0} * mMoveForce[1] + mUp * mMoveForce[2] - mVel*800.0f*dt;
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
          std::cout << (i==0 ? "X" : (i==1 ? "Y" : "Z")) << " no longer grounded: " << mIsGrounded << " : " << mGrounded << " : dPos: " << dPos << ", gdir: " << mGroundedDir << "\n";
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
          //std::cout << (i==0 ? "X" : (i==1 ? "Y" : "Z")) << " grounded -- checking: "
          //          << bMin << " : " << bMax << "\n";
          
	  bool hit = false;
	  for(bPos[i1] = bMin[i1]; bPos[i1] <= bMax[i1] && !hit; bPos[i1]++)
            for(bPos[i2] = bMin[i2]; bPos[i2] <= bMax[i2] && !hit; bPos[i2]++)
              {
                if(mWorld->get(bPos) != block_t::NONE)
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

          // std::cout << (i==0 ? "X" : (i==1 ? "Y" : "Z")) << " not grounded -- checking: "
          //           << bMin << " : " << bMax << "\n";
          
	  bool hit = false;
	  for(bPos[i1] = bMin[i1]; bPos[i1] <= bMax[i1] && !hit; bPos[i1]++)
            for(bPos[i2] = bMin[i2]; bPos[i2] <= bMax[i2] && !hit; bPos[i2]++)
              {
                if(mWorld->get(bPos) != block_t::NONE)
                  { // direction is grounded
                    hit = true;
                    mIsGrounded[i] = true;
                    mGrounded[i] = bPos[i];
                    mGroundedDir[i] = vdir[i];
                    mVel[i] = 0;
                    center[i] = bPos[i] + ((mGroundedDir[i] < 0) ?
                                           (mBox.size()[i] / 2.0f + 1) :
                                           (-mBox.size()[i] / 2.0f));
                    //std::cout << (i==0 ? "X" : (i==1 ? "Y" : "Z")) << " now grounded at: " << bPos[i] << ", center: " << center << "\n";
                    break;
                  }
              }
	}
    }
  mBox.setCenter(center);

  Point3i chunkPos = Point3i{std::floor(center[0]) / cChunk::sizeX, std::floor(center[1])/cChunk::sizeY, std::floor(center[2])/cChunk::sizeZ};// - Point3i{cChunk::sizeX, cChunk::sizeY, cChunk::sizeZ}/2;;
  //std::cout << "WORLD CENTER: " << mWorld->getCenter() << ", PLAYER POS: " << center << "\n";
  
  static Point3i lastChunkPos = chunkPos;
  if(chunkPos != lastChunkPos)
    {
      std::cout << "Moving center from " << lastChunkPos << " to " << chunkPos << "\n";
      mWorld->setCenter(chunkPos);
      lastChunkPos = chunkPos;
    }
  
  mVel[0] *= DRAG;
  mVel[1] *= DRAG;
}







#define GRAVITY 5000.0f





cGodPlayer::cGodPlayer(QObject *qparent, Vector3f pos, Vector3f forward, Vector3f up, cWorld *world)
  : cPlayer(qparent, pos, forward, up, world, Vector3f{0,0,0}, 256.0f) // far reach
{
  // fill up inventory supply of blocks
  //mInventory.resize(100, block_t::STONE);
}

void cGodPlayer::onUpdate(double dt)
{ // add force of gravity
  //mMoveForce[2] = -GRAVITY*dt;
}






#define EYE_HEIGHT 1.6f

cFpsPlayer::cFpsPlayer(QObject *qparent, Vector3f pos, Vector3f forward, Vector3f up, cWorld *world)
  : cPlayer(qparent, pos, forward, up, world, Vector3f{0, 0, EYE_HEIGHT - PLAYER_SIZE[2]*0.5f}, 4.0f) // short reach
{

}


void cFpsPlayer::jump(float strength)
{
  if(mIsGrounded[2])
    {
      std::cout << "Grounded!\n";
      std::cout << "Jumping --> accel: " << mAccel << ", vel: " << mVel << ", moveforce: " << mMoveForce << "\n";
      mVel[2] = strength*GRAVITY / 80;
      mIsGrounded[2] = false;
      mGrounded[2] = 0;
    }
  else
    {
      std::cout << "Not grounded!\n";
    }
}


void cFpsPlayer::onUpdate(double dt)
{ // add force of gravity
  mMoveForce[2] = -GRAVITY*dt;
}
