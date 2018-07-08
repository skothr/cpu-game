#ifndef TIMEDTHREAD_HPP
#define TIMEDTHREAD_HPP

#include <QThread>
#include <chrono>
#include <unistd.h>

#include "logging.hpp"

class QObject;

class cTimedThread : public QThread
{
  Q_OBJECT
public:
  cTimedThread(QObject *parent, bool skipSteps = false)
    : QThread(parent), mSkipSteps(skipSteps)
  { }

  virtual ~cTimedThread() { };
  
  void setInterval(int us)
  { mInterval = us; }

  void stop()
  {
    LOGD("Stopping physics thread...");
    mRunning = false;
  }

signals:
  void step(int dt_us);
  
private:
  bool mRunning = true;
  int mInterval = 0;
  bool mSkipSteps;

#define PRINT_INTERVAL 1000000  
  typedef std::chrono::high_resolution_clock CLOCK;
  typedef std::chrono::microseconds UNITS;
  
  void run()
  {
    auto startTime = CLOCK::now();
    auto lastTime = startTime;
    auto accum = std::chrono::duration_cast<UNITS>(startTime - startTime).count();
    int frames = 0;
    while(mRunning)
      {
	if(mInterval > 0)
	  {
	    auto now = CLOCK::now();
	    auto diff = std::chrono::duration_cast<UNITS>(now - lastTime).count();
	    accum += diff;
	
	    bool first = true;
	    while(accum > mInterval)
	      {
		if(!mSkipSteps || first)
		  {
		    emit step(mInterval);
		    frames++;
		  }
		accum -= mInterval;
		first = false;
	      }

	    auto elapsed = std::chrono::duration_cast<UNITS>(now - startTime).count();
	    if(elapsed >= PRINT_INTERVAL)
	      {
		LOGI("Physics running  (%.2f FPS)", frames / ((double)elapsed / 1000000.0));
		frames = 0;
		startTime = now;
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
