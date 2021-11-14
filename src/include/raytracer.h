#pragma once

#include "../maths/vec.hpp"
#include "./renderer.h"
// Rasterisation enough for now ...
// Now let's start with ray tracing and with light transport equations

// Just from the basic idea about raytracing, let's try rendering a 3D triangle into 2D space without perspective
// projection Will also be a good Math warmup
//
// All maths goes into this header for now

// Lets implement simple ray and triangle intersection

struct Plane
{
    Vec3f normal; // normal unit vector of the plane .. essentially A,B,C of Ax+By+Cz = D
    float constantD;

    Plane(Vec3f const &a, Vec3f const &b, Vec3f const &c) // Equation of a plane from given three points
                                                          // Calculate the normal vector first
        : normal{Vec3f::Cross(b - a, c - b)}, constantD{Vec3f::Dot(normal, a)} {};
};

class Ray
{
  public:
    Vec3f origin;
    Vec3f direction; // Unit vector preferred
    float t;         // parametric parameterization of ray

    Ray(Vec3f origin, Vec3f dir) : origin{origin}, direction{dir}, t{0}
    {
    }

    Vec3f PathPoint()
    {
        return origin + t * direction;
    }

    Vec3f PathPoint(float t)
    {
        return origin + t * direction;
    }
    [[nodiscard]] Vec3f Intersect(Plane const &plane)
    {
        // Checks for ray plane intersection
        // A ray either:
        //                  1. Doesn't intersect a plane, if its parallel to the plane
        //                  2. Intersects exactly at one point (most probable)
        //                  3. Intersects everywhere (lie wholly on the plane)

        if (!Vec3f::Dot(plane.normal, direction)) // floating point inaccuracies ignored
        {
            // The plane is parallel to the traveling ray
            // Check further for if line lies in the plane
            // For that, line's orgin must lie in the plane

            // Using vector geometry, this is determined by
            if (Vec3f::Dot(plane.normal, origin) == plane.constantD)
            {
                // The line lies in the plane
                // What to return? Whole plane? or Line or what?
            }
            else
                return {};
            // Returns empty vector .. No intersection
        }
        // Else check for the intersection of the ray and the plane using some vector geometry
        // Time to do some maths
        // Ok done
        float t =
            (plane.constantD - Vec3f::Dot(plane.normal, this->origin)) / (Vec3f::Dot(plane.normal, this->direction));
        // Let's check if this formula is actually correct... but how?
        // Let's try rendering a triangle
        return PathPoint(t);
        // There should be a better way to check for the intersection.. leave it for now
    }
};

// Should we go for ray plane intersection or the triangle one?
// If we go for ray plane intersection and then, calculate whether point is inside the triangle using Half-Plane
// method, if it is inside render else discard.
namespace RayTracer
{
void RenderTriangle(Vec3f const &a, Vec3f const &b, Vec3f const &c);
}