#include "player.hpp"
#include "logging.hpp"


#define DRAG 0.98f
#define GRAVITY 80.0f
#define EYE_HEIGHT 1.6f
#define TOUCH_RADIUS 4.0f

static const Vector3f PLAYER_SIZE{0.9, 0.9, 2};
const Vector3f cPlayer::mEyeOffset = Vector3f{0, 0, EYE_HEIGHT - PLAYER_SIZE[2]*0.5f};



// pos should be the BOTTOM-CENTER of the player's bounding box.
cPlayer::cPlayer(Vector3f pos, Vector3f forward, Vector3f up, cChunkMap *map)
  : cCollidable(pos + Vector3f{0,0,PLAYER_SIZE[2]*0.5f}, PLAYER_SIZE),
    mForward(forward.normalized()), mUp(up.normalized()),
    mRight(crossProduct(forward, up).normalized()),
    mMap(map), mHighlightModel("./res/highlight.obj")
{
  
}
void cPlayer::addForce(float right, float forward)
{
  mMoveForce += Vector3f{right, forward, 0.0};
}
void cPlayer::setXForce(float right)
{
  mMoveForce[0] = right;
}
void cPlayer::setYForce(float forward)
{
  mMoveForce[1] = forward;
}

void cPlayer::jump(float strength)
{
  if(mGrounded[2] != 0)
    {
      std::cout << "Grounded!\n";
      std::cout << "Jumping --> accel: " << mAccel << ", vel: " << mVel << ", moveforce: " << mMoveForce << "\n";
      mVel[2] = strength*GRAVITY;
    }
  else
    {
      std::cout << "Not grounded!\n";
    }
}

void cPlayer::rotate(float pitch, float yaw)
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

bool cPlayer::interact()
{
  LOGD("PLAYER INTERACTING");
  if(mSelected)
    {
      LOGD("PLAYER GOT BLOCK!");
      LOGD("  TYPE: %d", (int)mSelected->type);
      mInventory.push_back(mSelected->type);
      mMap->set(mSelectedPos, block_t::NONE);
      mSelected = nullptr;
    }
  
  return true;
}

void cPlayer::place()
{
  if(mInventory.size() > 0)
    {
      if(mSelected)
	{
	  LOGD("PLAYER PLACED BLOCK at: ");
	  std::cout << mSelectedPos << " (face " << mSelectedFace << ")\n";
	  if(mMap->set(mSelectedPos + mSelectedFace, mInventory.back()))
	    {
	      mInventory.pop_back();
	    }
	}
    }
}

void cPlayer::update(double dt)
{
  Vector3f right = crossProduct(mForward, mUp).normalized();
  
  if(mGrounded[2] == 0)
  {
    mMoveForce[2] += -GRAVITY*dt;
  }
  else
    {
      mMoveForce[2] = -GRAVITY*dt;
      //mAccel[2] = 0;
    }
  
  mAccel = Vector3f{right[0], right[1], 0.0}  * mMoveForce[0] + Vector3f{mForward[0], mForward[1], 0} * mMoveForce[1] + mUp * mMoveForce[2] - mVel*800.0f*dt;
  

  //std::cout << "move force: " << mMoveForce << ", accel: " << mAccel << ", vel: " << mVel << "\n";
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

  Point3f correction;

  for(int i = 0; i < 3; i++)
    {
      if(mGrounded[i] != 0)
	{
	  if(mVel[i] < 0 && (newMini[i] <= mGrounded[i] &&
			     newMaxi[i] > mGrounded[i] ))
	    {
	      correction[i] += mGrounded[i] - newMin[i];
	    }
	  else if(mVel[i] > 0 && (newMaxi[i] >= mGrounded[i] &&
				  newMini[i] < mGrounded[i] ))
	    {
	      correction[i] -= mGrounded[i] - newMax[i];
	    }
	  else
	    {
	      mGrounded[i] = 0;
	    }
	}
    }
  
  Point3i before{ mVel[0] < 0 ? min[0] : (mVel[0] > 0 ? max[0] : 0),
      mVel[1] < 0 ? min[1] : (mVel[1] > 0 ? max[1] : 0),
      mVel[2] < 0 ? min[2] : (mVel[2] > 0 ? max[2] : 0) };
  //std::cout << "before pos: " << mBox.center() << " min: " << min << " max: " << max << " vel " << mVel << "\n";
  mBox.move(dPos);
  min = Point3i{std::floor(mBox.minPoint()[0]),
		std::floor(mBox.minPoint()[1]),
		std::floor(mBox.minPoint()[2]) };
  max = Point3i{std::ceil(mBox.maxPoint()[0])-1,
		std::ceil(mBox.maxPoint()[1])-1,
		std::ceil(mBox.maxPoint()[2])-1};
  Point3i after{ mVel[0] < 0 ? min[0] : (mVel[0] > 0 ? max[0] : 0),
		 mVel[1] < 0 ? min[1] : (mVel[1] > 0 ? max[1] : 0),
		 mVel[2] < 0 ? min[2] : (mVel[2] > 0 ? max[2] : 0) };
  //std::cout << "after pos: " << mBox.center() << " min: " << min << " max: " << max << " vel " << mVel << "\n";
  mBox.move(correction);
  //std::cout << "corrected pos: " << mBox.center() << " vel " << mVel << "\n";

  Point3f center = mBox.center();

  const char* letters[3] = {"X", "Y", "Z"};

  Point3i vdir{mVel[0] < 0 ? -1 : 1,
	       mVel[1] < 0 ? -1 : 1,
	       mVel[2] < 0 ? -1 : 1 };
  for(int i = 0; i < 3; i++)
    {
      if(before[i] != after[i])
	{
	  //std::cout << "CHECKING " << letters[i] << " AXIS  (before: " << before[i] << ", after: " << after[i] << ")\n";
	  
	  Point3i bPos = after;
	  Point3i bMin = min;
	  Point3i bMax = max;
	  int i1 = (i+1)%3;
	  int i2 = (i+2)%3;
	  if(mGrounded[i1])
	    {
	      bMin[i1] = (vdir[i1] < 0 ? mGrounded[i1] : mGrounded[i1] - 1);// - 1);
	      bMax[i1] = bMin[i1];
	      //std::cout << "  " << letters[i1] << " grounded at " << mGrounded[i1] << " --> checking Z=" << bMin[i1] << "\n";
	    }
	  if(mGrounded[i2])
	    {
	      bMin[i2] = (vdir[i2] < 0 ? mGrounded[i2] : mGrounded[i2] - 1);// - 1);
	      bMax[i2] = bMin[i2];
	      //std::cout << "  " << letters[i2] << " grounded at " << mGrounded[i2] << " --> checking Z=" << bMin[i2] << "\n";
	    }
	  bool hit = false;
	  for(int bp1 = bMin[i1]; bp1 <= bMax[i1]; bp1++)
	    {
	      bPos[i1] = bp1;
	      for(int bp2 = bMin[i2]; bp2 <= bMax[i2]; bp2++)
		{
		  bPos[i2] = bp2;
		  //std::cout << "  Checking block at " << bPos << "...";
		  cBlock* b = mMap->get(bPos);
		  if(b && b->type != block_t::NONE)
		    {
		      mGrounded[i] = vdir[i] > 0 ? bPos[i] : (bPos[i] + 1);
		      center[i] = mGrounded[i] - mBox.size()[i] / 2.0f * (vdir[i] < 0 ? -1 : 1);
		      mVel[i] = 0;
		      //std::cout << "HIT! grounded: " << mGrounded << ", center: " << center << "\n";
		      hit = true;
		    }
		  else
		    {
		      //std::cout << "NO BLOCK\n";
		    }
		}
	    }
	  if(!hit)
	    {
	      mGrounded[i] = 0;
	    }
	}
      else
	{
	  //std::cout << letters[i] << " NO CHANGE --> b: " << before << ", a: " << after << "\n";
	}
    }
  //std::cout << "grounded: " << mGrounded << "\n";
  mBox.setCenter(center);
  
  mVel[0] *= DRAG;
  mVel[1] *= DRAG;

}

Vector3f cPlayer::getEye() const
{
  return (mForward*(1.0 - std::abs(mPitch)) + mUp * (mPitch)).normalized();
}

Matrix4 cPlayer::getView()
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

void cPlayer::initGL(cShader *shader)
{
  mHighlightModel.initGL(shader);
}
void cPlayer::cleanupGL()
{
  mHighlightModel.cleanupGL();
}

void cPlayer::render(cShader *shader, Matrix4 pvm)
{
  if(!mMap->rayCast(mBox.center() + mEyeOffset, getEye(), TOUCH_RADIUS,
		    mSelected, mSelectedPos, mSelectedFace ))
    {
      mSelected = nullptr;
    }
  if(mSelected)
    {
      //LOGI("Selected block --> (%d, %d, %d)", mSelectedPos[0], mSelectedPos[1], mSelectedPos[2]);
      mHighlightModel.render(shader, pvm*matTranslate(mSelectedPos[0], mSelectedPos[1], mSelectedPos[2]));
      //std::cout << "HIGHLIGHTED: " << mHighlighted << "\n";
    }
}
