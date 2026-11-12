#pragma once
#include <SDL2/SDL.h>
#include <engine/Steering.h>
#include <engine/WeightedBlending.h>
#include <engine/PrioritizedList.h>

// CAMBIO POR FUNCIONAMIENTO
// ----------------------------------

#include <engine/Flee.h>
#include <engine/Seek.h>
#include <engine/Arrive.h>

// ----------------------------------

class BehaviourSelector
{
public:
	BehaviourSelector()
	{
		blend_.Add(&seek_, 1.0f);
		blend_.Add(&flee_, 0.5f);

		prio_.Add(&flee_);     
		prio_.Add(&seek_);
	}

	bool OnKey(SDL_Keycode key, World& world)
	{
		switch (key) {
		case SDLK_1: Select(&seek_, world); return true;
		case SDLK_2: Select(&flee_, world); return true;
		case SDLK_3: Select(&arrive_, world); return true;
		case SDLK_4: Select(&blend_, world); return true;
		case SDLK_5: Select(&prio_, world); return true;
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
			a.behaviour = b; // VARIABLE AÑADIDA A STEERING.H PARA QUE FUNCIONE
	}

	// CAMBIO POR FUNCIONAMIENTO
	// ----------------------------------

	// INCLUDES AÑADIDOS PARA QUE FUNCIONE

	Seek seek_;
	Flee flee_;
	Arrive arrive_;
	WeightedBlending blend_;
	PrioritizedList prio_;

	// ----------------------------------

	SteeringBehaviour* current_ = &seek_;
};