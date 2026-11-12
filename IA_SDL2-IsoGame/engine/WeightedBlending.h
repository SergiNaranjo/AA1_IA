#pragma once
#include "CompositeBehaviour.h"

class WeightedBlending : public CompositeBehaviour 
{
public:
	Vector2 GetSteering(const Agent& a, const World& w) override
	{
		Vector2 total(0.0f, 0.0f);
		for (const Child& c : children_)
		{
			total += c.behaviour->GetSteering(a, w) * c.weight;
		}
		return total.truncated(a.maxForce);
	}
};