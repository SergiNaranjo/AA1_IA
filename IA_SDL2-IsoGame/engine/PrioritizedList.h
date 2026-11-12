#pragma once
#include "CompositeBehaviour.h"

class PrioritizedList : public CompositeBehaviour
{
public:
    float epsilon = 1e-4f;   

    Vector2 GetSteering(const Agent& a, const World& w) override
    {
        for (const Child& c : children_)         
        {
            Vector2 s = c.behaviour->GetSteering(a, w);
            if (s.lengthSquared() > epsilon)      
                return s;
        }
        return Vector2(0.0f, 0.0f);
    }
};