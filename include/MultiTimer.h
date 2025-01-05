#pragma once

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
