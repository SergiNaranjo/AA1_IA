#pragma once
#include "Steering.h"

class Seek : public SteeringBehaviour
{
    

public:
    Vector2 GetSteering(const Agent& a, const World& w) override
    {
        if (!a.hasTarget) return Vector2(0.0f, 0.0f);

        Vector2 desired = (a.target - a.position).normalized() * a.maxSpeed;
        return (desired - a.velocity).truncated(a.maxForce);
    }
};
