#ifndef MULTITIMER_H
#define MULTITIMER_H

#include "Arduino.h"


class MultiTimer
{
  public:
  	MultiTimer(unsigned long);
 	void tick();
	void reset();
	bool expired;
	
  private:
  	unsigned long _startMillis;
    unsigned long _interval = 0;
};



#endif
