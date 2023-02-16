#include "MULTITIMER.h"

MultiTimer::MultiTimer(unsigned long interval)
{
 _interval = interval;
 _startMillis = millis();
}

void MultiTimer::tick()
{
	if (millis() - _startMillis >= _interval)
	{
		_startMillis = millis();
		expired = true;
	}
	else expired = false;
}

void MultiTimer::reset()
{
	_startMillis = millis();
}