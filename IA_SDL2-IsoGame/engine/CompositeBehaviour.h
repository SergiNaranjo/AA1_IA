#pragma once
#include "Steering.h"

class CompositeBehaviour : public SteeringBehaviour
{
public:
	void Add(SteeringBehaviour* b, float weight = 1.0f);
	void Remove(SteeringBehaviour* b);
	void Replace(SteeringBehaviour* old, SteeringBehaviour* nw); //Para cambiar Seek y arrive ya que son muy parecidos

protected:
	struct Child { SteeringBehaviour* behaviour; float weight; };
	std::vector<Child> children_;
};
