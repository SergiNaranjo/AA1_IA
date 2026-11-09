#pragma once
#include "Steering.h"

class Arrive : public SteeringBehaviour
{
public:
    float slowRadius = 192.0f;
    float stopRadius = 4.0f;

    Vector2 GetSteering(const Agent& a, const World& w) override
    {
        if (!a.hasTarget) return Vector2(0,0);

        Vector2 toTarget = a.target - a.position;
        float distance = toTarget.length();

        if (distance < stopRadius) 
        {
            return (a.velocity * -1.0f).truncated(a.maxForce); 
        }

        float speed = a.maxSpeed;
        if (distance < slowRadius)
            speed = a.maxSpeed * (distance / slowRadius);

        Vector2 desired = toTarget.normalized() * speed;
        return (desired - a.velocity).truncated(a.maxForce);
    }
};
