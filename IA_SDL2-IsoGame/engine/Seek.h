#pragma once
#include "Steering.h"

class Seek : public SteeringBehaviour
{
    

public:
    Vector2 GetSteering(const Agent& a, const World& w) override
    {
        // Desired velocity: full speed straight at the target.
        Vector2 desired = (a.target - a.position).normalized() * a.maxSpeed;
        // Steering force = desired velocity minus current velocity, clamped.
        return (desired - a.velocity).truncated(a.maxForce);
    }
};
