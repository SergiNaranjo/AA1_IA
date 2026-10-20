#ifndef ENGINE_VECTOR2_HPP
#define ENGINE_VECTOR2_HPP
#include <cmath>

// FILE: minimal 2D vector for world-space maths (positions, velocities, forces).
// FITS IN: every AI computation (Agent, World, steering) uses this flat
// Cartesian space; the isometric projection only appears at draw time in Game.
// EXTEND: add more ops here (dot/cross/rotate/lerp) if a steering behaviour
// needs them - keep it header-only and textbook-plain, no engine concepts.
struct Vector2
{
    float x = 0.0f;
    float y = 0.0f;

    Vector2() = default;
    Vector2(float x, float y) : x(x), y(y) {}

    Vector2 operator+(const Vector2 &o) const { return Vector2(x + o.x, y + o.y); }
    Vector2 operator-(const Vector2 &o) const { return Vector2(x - o.x, y - o.y); }
    Vector2 operator*(float s) const { return Vector2(x * s, y * s); }
    Vector2 operator/(float s) const { return Vector2(x / s, y / s); }

    Vector2 &operator+=(const Vector2 &o) { x += o.x; y += o.y; return *this; }
    Vector2 &operator-=(const Vector2 &o) { x -= o.x; y -= o.y; return *this; }

    float length() const { return std::sqrt(x * x + y * y); }
    float lengthSquared() const { return x * x + y * y; }

    // Summary: unit vector in the same direction as this one.
    // Inputs: none beyond *this (works for any x/y, including 0,0).
    // Output: new Vector2; no side effects.
    // Errors: none - WATCH OUT: 1e-6f below is a "close enough to zero" guard
    // so a near-zero vector returns (0,0) instead of dividing by ~0.
    Vector2 normalized() const
    {
        float len = length();
        if (len < 1e-6f) {
            return Vector2(0.0f, 0.0f);
        }
        return Vector2(x / len, y / len);
    }

    // Summary: same direction, magnitude clamped to at most maxLength.
    // Used by Agent to cap steering force/velocity every frame.
    // Output: new Vector2; no side effects. Errors: none.
    Vector2 truncated(float maxLength) const
    {
        float len = length();
        if (len > maxLength && len > 1e-6f) {
            return Vector2(x / len * maxLength, y / len * maxLength);
        }
        return *this;
    }
};

inline float distance(const Vector2 &a, const Vector2 &b)
{
    return (a - b).length();
}

#endif // ENGINE_VECTOR2_HPP
