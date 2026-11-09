#pragma once
#include <SDL2/SDL.h>
#include <engine/Steering.h>

class BehaviourSelector
{
public:
	bool OnKey(SDL_Keycode key, World& world) 
	{
		switch (key) {
		case SDLK_1: Select(&seek_, world); return true;
		case SDLK_2: Select(&flee_, world); return true;
		case SDLK_3: Select(&arrive_, world); return true;
		}
		return false;

		
	}

	void Apply(World& world)
	{
		Select(current_, world);
	}

private:
	void Select(SteeringBehaviour* b, World& w)
	{
		current_ = b;
		for (Agent& a : w.agents) 
			a.behaviour = b;
	}

	Seek seek_;
	Flee flee_;
	Arrive arrive_;
	SteeringBehaviour* current_ = &seek_;
};