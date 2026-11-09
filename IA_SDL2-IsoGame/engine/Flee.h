#pragma once
#include "Steering.h"

class Flee : public SteeringBehaviour
{
public:
    Vector2 GetSteering(const Agent& a, const World& w) override
    {
        //Por si no tiene objetivo que no huya nunca el mingui
        if (!a.hasTarget) return Vector2(0.0f, 0.0f);

        // Desired velocity: full speed straight at the target.
        Vector2 desired = (a.position - a.target).normalized() * a.maxSpeed;
        // Steering force = desired velocity minus current velocity, clamped.
        return (desired - a.velocity).truncated(a.maxForce);
    }
};
