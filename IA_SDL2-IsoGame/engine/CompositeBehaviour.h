#pragma once
#include "Steering.h"

class CompositeBehaviour : public SteeringBehaviour
{
public:
	void Add(SteeringBehaviour* b, float weight = 1.0f)
	{
		children_.push_back({ b, weight });
	}
	void Remove(SteeringBehaviour* b)
	{
		for (size_t i = 0; i < children_.size(); ++i)
		{
			if (children_[i].behaviour == b)
			{
				children_.erase(children_.begin() + i);
				return;
			}
		}
	}
	void Replace(SteeringBehaviour* old, SteeringBehaviour* nw)
	{
		for (Child& c : children_)
			if (c.behaviour == old) c.behaviour = nw;
	}//Para cambiar Seek y arrive ya que son muy parecidos

protected:
	struct Child 
	{ 
		SteeringBehaviour* behaviour; 
		float weight; 
	};

	std::vector<Child> children_;
};
