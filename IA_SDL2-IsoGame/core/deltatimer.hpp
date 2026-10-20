#ifndef __DELTA_TIMER_H_
#define __DELTA_TIMER_H_
#include <SDL2/SDL.h>

// FILE: frame-delta-time singleton (seconds since last frame).
// FITS IN: Game::run() updates it once per frame; anything wanting dt reads
// DeltaTimer::Instance()->deltaTimerGetDeltatime() rather than measuring its own.
// IMPORTANT INVARIANT: call deltaTimerInit() once before the first
// deltaTimerUpdate()/deltaTimerGetDeltatime(), or oldTick/tick are garbage.
class DeltaTimer
{
    public:
    ~DeltaTimer(){};
    static DeltaTimer * s_pInstance;

    private:
    DeltaTimer(){}; //Singleton private constructor
    Uint32 oldTick;
    Uint32 tick;
    double deltaTime;

    public:
    static DeltaTimer *Instance();
    void deltaTimerInit();
    void deltaTimerUpdate();
    double deltaTimerGetDeltatime();

};

#endif // __DELTA_TIMER_H_

