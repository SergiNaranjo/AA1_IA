// FILE: DeltaTimer implementation - see deltatimer.hpp for the contract.
#include <SDL2/SDL.h>
#include "deltatimer.hpp"

DeltaTimer *DeltaTimer::s_pInstance = 0;

DeltaTimer *DeltaTimer::Instance()
{
    if (s_pInstance == 0)
	{
		s_pInstance = new DeltaTimer();
	}
	return s_pInstance;
}

void DeltaTimer::deltaTimerInit()
{
    //initialize the values to 0.0
    deltaTime = 0.0;
    oldTick = 0.0;
    tick = 0.0;
}

void DeltaTimer::deltaTimerUpdate()
{
    //get the old tick time
    oldTick = tick;

    //get current tick time
    tick = SDL_GetTicks();

    //get time duration between ticks and divide by 1000
    //to get number of seconds since last frame
    deltaTime = (tick - oldTick) / 1000.0;
}

double DeltaTimer::deltaTimerGetDeltatime()
{
    //return the delta time
    return deltaTime;
}
