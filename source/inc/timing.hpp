#ifndef TIMING_HPP
#define TIMING_HPP

#include <chrono>
#include <string>
#include <iostream>


#define CLOCK_T std::chrono::high_resolution_clock
#define TIME_T std::chrono::time_point<CLOCK_T>
#define DURATION_T std::chrono::duration<double>
#define DURATION_UNITS std::chrono::duration_cast<std::chrono::nanoseconds>

struct cTimer
{
  // interval is time to wait before averaging frame rate
  cTimer(const std::string &name, double interval)
    : name(name), interval(interval)
  { startTime = CLOCK_T::now(); }

  std::string name;
  TIME_T startTime;
  TIME_T last;
  TIME_T now;

  int numFrames = 0;
  double elapsed = 0.0;
  double interval;
  double fps = 0.0;
  bool started = false;

  void reset()
  {
    elapsed = 0.0;
    fps = 0.0;
    numFrames = 0;
    started = false;
    startTime = CLOCK_T::now();
  }
  void start()
  {
    if(!started)
      {
	startTime = CLOCK_T::now();
	started = true;
      }
  }
  
  // returns elapsed time
  double update(bool printFps = false)
  {
    if(!started)
      { return -1.0; }
    now = CLOCK_T::now();
    elapsed = DURATION_UNITS(now - startTime).count() / 1000000000.0;;
    if(interval > 0.0 || elapsed >= interval)
      {
	fps = (numFrames / elapsed);

	if(printFps)
	  {
	    std::cout << "Timer '" << name << "': " << numFrames << " frames in "
		      << elapsed << "s (" << fps << " fps)\n";
	  }
	
	numFrames = 0;
	startTime = now;
	last = now;
	return elapsed;
      }
    else
      {
	numFrames++;
	elapsed = (now - startTime).count() / 1000000000.0;;
	last = now;
	return elapsed;
      }
  }
};
struct cFrameTimer
{
  // interval is number of frames to wait before averaging
  cFrameTimer(const std::string &name, int frameInterval)
    : name(name), interval(frameInterval)
  { startTime = CLOCK_T::now(); }

  std::string name;
  TIME_T startTime;
  TIME_T last;
  TIME_T now;

  int numFrames = 0;
  double elapsed = 0.0;
  int interval;
  double fps = 0.0;
  bool started = false;
  
  void reset()
  {
    elapsed = 0.0;
    fps = 0.0;
    numFrames = 0;
    started = false;
  }
  
  void start()
  {
    if(!started)
      {
	startTime = CLOCK_T::now();
	started = true;
      }
  }
  
  inline double update(bool printFps = false)
  {
    now = CLOCK_T::now();
    elapsed = DURATION_UNITS(now - startTime).count() / 1000000000.0;
    if(interval > 0 || numFrames >= interval)
      {
	fps = numFrames / elapsed;
	if(printFps)
	  {
	    std::cout << "Timer '" << name << "': " << numFrames << " frames in "
		      << elapsed << "s (" << fps << " fps)\n";
	  }
	numFrames = 0;
	startTime = now;
	last = now;
	return elapsed;
      }
    else
      {
	numFrames++;
	double frameElapsed = DURATION_UNITS(now - startTime).count() / 1000000000.0;
	last = now;
	return frameElapsed;
      }
  }
};




#endif // TIMING_HPP
