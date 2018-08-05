#ifndef TIMEDTHREAD_HPP
#define TIMEDTHREAD_HPP

#include <thread>
#include <chrono>
#include <functional>
#include <unistd.h>

#include "logging.hpp"

class cTimedThread
{
public:
  typedef std::function<void(double)> stepFunc_t;
  
  cTimedThread(const std::string &name, const stepFunc_t &stepCallback, bool skipSteps = false)
    : mName(name), mStepCallback(stepCallback), mSkipSteps(skipSteps)
  { }

  virtual ~cTimedThread() { };

  
  void setInterval(int us)
  { mInterval = us; }

  void start()
  {
    if(!mRunning)
      {
        LOGD("Starting timed thread '%s'...", mName.c_str());
        mRunning = true;
        mThread = std::thread(&cTimedThread::run, this);
      }
  }
  void join()
  { 
    if(mRunning)
      {
        LOGD("Joining timed thread '%s'...", mName.c_str());
        mRunning = false;
        mThread.join();
      }
  }
  
private:
  bool mRunning = false;
  int mInterval = 0;
  bool mSkipSteps;
  std::string mName;
  std::thread mThread;
  stepFunc_t mStepCallback;

#define PRINT_INTERVAL 1000000  
  typedef std::chrono::high_resolution_clock CLOCK;
  typedef std::chrono::microseconds UNITS;
  
  void run()
  {
    auto startTime = CLOCK::now();
    auto lastTime = startTime;
    double accum = std::chrono::duration_cast<UNITS>(startTime - startTime).count();
    double lastDiff = accum;
    int frames = 0;

    if(!mStepCallback)
      {
	LOGW("Timed thread '%s' step function is null.", mName.c_str());
	return;
      }
    
    while(mRunning)
      {
	if(mInterval > 0)
	  {
	    auto now = CLOCK::now();
	    double diff = std::chrono::duration_cast<UNITS>(now - lastTime).count();

	    if((diff - lastDiff) > 1000000) // more than 1ms difference b/w frames
	      {
		LOGW("Frame %fs longer than last frame.", (diff - lastDiff));
	      }
	    
	    accum += diff;
	
	    while(accum > mInterval)
	      {
		mStepCallback(mInterval);
		frames++;
		accum -= mInterval;
		if(mSkipSteps)
		  {
		    accum = 0.0;
		    break;
		  }
	      }

	    if(frames > 0)
	      {
		double elapsed = std::chrono::duration_cast<UNITS>(now - startTime).count();
		if(elapsed >= PRINT_INTERVAL)
		  {
		    LOGI("Timed thread '%s' running  (%.2f FPS)", mName.c_str(),
			 frames / ((double)elapsed / 1000000.0));
		    frames = 0;
		    startTime = now;
		  }
	      }
	    
	    lastTime = now;
	    usleep(1000);
	  }
	else
	  { usleep(1000); }
      }
  }
};


#endif // TIMEDTHREAD_HPP
