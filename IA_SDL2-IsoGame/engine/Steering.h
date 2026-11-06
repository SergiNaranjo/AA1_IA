#pragma once
#include <vector>
#include "Vector2.hpp"
#include "Agent.hpp"
#include "World.hpp"

class SteeringBehaviour {
public:
	virtual ~SteeringBehaviour() = default; //Destructor
	virtual Vector2 GetSteering(const Agent& a, const World& w) = 0;
};