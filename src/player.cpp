#include "player.hpp"
#include "logging.hpp"


#define DRAG 0.98f
#define GRAVITY 80.0f
#define EYE_HEIGHT 2.0f
#define TOUCH_RADIUS 4.0f

static const Vector3f PLAYER_SIZE{0.9, 0.9, 2};
const Vector3f cPlayer::mEyeOffset = Vector3f{0, 0, EYE_HEIGHT - PLAYER_SIZE[2]*0.5f};



// pos should be the BOTTOM-CENTER of the player's bounding box.
cPlayer::cPlayer(Vector3f pos, Vector3f forward, Vector3f up, cChunk *chunk)
  : cCollidable(pos + Vector3f{0,0,PLAYER_SIZE[2]*0.5f}, PLAYER_SIZE),
    mForward(forward.normalized()), mUp(up.normalized()),
    mRight(crossProduct(forward, up).normalized()),
    mChunk(chunk), mHighlightModel("./res/highlight.obj")
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
  std::cout << "Jumping...\n";
  if(mGrounded[2])
    {
      std::cout << "Grounded!\n";
      //mMoveForce += Vector3f{0, 0, strength*GRAVITY};
      mVel[2] += strength*GRAVITY;
      mGrounded[2] = false;
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
      mInventory.push_back(mSelected);
      mChunk->pickBlock(mSelectedPos);
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
	  if(mChunk->placeBlocks({mSelectedPos + mSelectedFace}, {mInventory.back()}))
	    {
	      mInventory.pop_back();
	    }
	}
    }
}

void cPlayer::update(double dt)
{
  Vector3f right = crossProduct(mForward, mUp).normalized();
  
  mAccel = Vector3f{right[0], right[1], 0.0}  * mMoveForce[0] + Vector3f{mForward[0], mForward[1], 0} * mMoveForce[1] + mUp * mMoveForce[2] - mVel*800.0f*dt;
  
  if(!mGrounded[2])
    {
      mMoveForce += Vector3f{0, 0, -GRAVITY}*dt;
    }

  //std::cout << "move force: " << mMoveForce << ", accel: " << mAccel << ", vel: " << mVel << "\n";
  mVel += mAccel * dt;
  Vector3f dPos = mVel * dt;
  Point3f oldPos = mBox.center();
  
  mBox.move(dPos);
  std::vector<cBoundingBox> blocks = mChunk->collides(mBox);
  if(blocks.size() > 0)
    {
      std::cout << std::fixed << std::setprecision(5);
      std::vector<Vector3f> corrections = mChunk->correction(mBox, blocks, dPos, mGrounded);

      Vector3f totalCorr;
      int numCorr = 0;
      for(auto &corr : corrections)
	{
	  corr -= totalCorr;

	  totalCorr[0] = (dPos[0] < 0 ? std::min(corr[0], totalCorr[0]) : (dPos[0] > 0 ? std::max(corr[0], totalCorr[0]) : 0));
	  totalCorr[1] = (dPos[1] < 0 ? std::min(corr[1], totalCorr[1]) : (dPos[1] > 0 ? std::max(corr[1], totalCorr[1]) : 0));
	  totalCorr[2] = (dPos[2] < 0 ? std::min(corr[2], totalCorr[2]) : (dPos[2] > 0 ? std::max(corr[2], totalCorr[2]) : 0));
	}
      //std::cout << "DPOS: " << dPos << ", CORR: " << totalCorr << "\n";
      
      for(int i = 0; i < 3; i++)
	{
	  if(totalCorr[i] != 0.0f)
	    {
	      //std::cout << (i == 0 ? "X" : (i==1 ? "Y" : "Z")) << " GROUNDED\n";
	      mGrounded[i] = 1;
	      mVel[i] = 0.0f;
	      mAccel[i] = 0.0f;
	      if(i == 2)
		mMoveForce[2] = 0.0f;
	    }
	  else if(totalCorr[i] == 0.0f)
	    {
	      //std::cout << (i == 0 ? "X" : (i==1 ? "Y" : "Z")) << " UN-GROUNDED\n";
	      mGrounded[i] = 0;
	    }
	}
      mBox.move(-totalCorr);
    }
  else
    {
      std::vector<cBoundingBox> blockEdges = mChunk->collides(mBox, true);
      if(blockEdges.size() == 0)
	{
	  mGrounded[0] = 0;
	  mGrounded[1] = 0;
	  mGrounded[2] = 0;
	  //std::cout << "ALL SIDES UN-GROUNDED\n";
	}
    }
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
  if(!mChunk->closestIntersection(mBox.center() + mEyeOffset, getEye(), TOUCH_RADIUS, mSelected, mSelectedPos, mSelectedFace))
    {
      mSelected = nullptr;
    }
  if(mSelected)
    {
      LOGI("Selected block --> (%d, %d, %d)", mSelectedPos[0], mSelectedPos[1], mSelectedPos[2]);
      mHighlightModel.render(shader, pvm*matTranslate(mSelectedPos[0], mSelectedPos[1], mSelectedPos[2]));
      //std::cout << "HIGHLIGHTED: " << mHighlighted << "\n";
    }
}
