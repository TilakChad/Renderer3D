#pragma once

#include "../include/render.h"
#include "../maths/vec.hpp"
#include <vector>



namespace Shape
{

class Shapes
{
};
class Cylinder
{
  public:
    static RenderInfo offload(float radius, float height)
    {
        std::vector<Pipeline3D::VertexAttrib3D> vertices;
        std::vector<uint32_t>                   indices;

        constexpr float                         pi    = 3.141592f;
        float                                   hStep = 0.25f;
        float                                   tStep = 0.25f;

        float                                   h     = 0;
        float                                   t     = 0;
        uint32_t                                index = 0;

        while (h < height)
        {
            for (t = 0; t < 2 * pi; t += tStep)
            {
                Pipeline3D::VertexAttrib3D v0;
                v0.Position = Vec4f(radius * cos(t), h, radius * sin(t), 1.0f);
                v0.Color    = Vec4f(0.4f, 0.4f, 0.4f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f(radius * cos(t + tStep), h, radius * sin(t + tStep), 1.0f);
                // v0.Color    = Vec4f(0.0f, 1.0f, 0.0f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f(radius * cos(t + tStep), h + hStep, radius * sin(t + tStep), 1.0f);
                // v0.Color    = Vec4f(0.0f, 0.0f, 1.0f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f(radius * cos(t), h + hStep, radius * sin(t), 1.0f);
                // v0.Color    = Vec4f(1.0f, 1.0f, 0.0f, 0.0f);
                vertices.push_back(v0);
                indices.insert(indices.end(), {index + 2, index + 1, index, index + 3, index + 2, index});
                index += 4;
            }
            h = h + hStep;
        }
        return RenderInfo(std::move(vertices), std::move(indices), RenderDevice::MergeMode::COLOR_MODE, 0);
    }
};
class Sphere
{
  public:
    static RenderInfo offload(float radius, float phiStep = 0.25f, float theStep = 0.25f, Vec4f color = Vec4f(0.4f,0.4f,0.4f,0.0f))
    {
        // oof .. wasted too much time here due to incorrect winding order
        std::vector<Pipeline3D::VertexAttrib3D> vertices;
        std::vector<uint32_t>                   indices;

        constexpr float                         pi    = 3.14159265f;

        uint32_t                                index = 0;

        for (float x = pi / 2; x >= -pi / 2; x -= theStep)
        {
            float theta = pi / 2 - x;
            for (float phi = -pi; phi <= pi; phi += phiStep)
            {
                Pipeline3D::VertexAttrib3D v0;
                v0.Position =
                    Vec4f(radius * sin(theta) * cos(phi), radius * cos(theta), radius * sin(theta) * sin(phi), 1.0f);
                v0.Color = color;
                vertices.push_back(v0);
                v0.Position = Vec4f(radius * sin(theta - theStep) * cos(phi), radius * cos(theta - theStep),
                                    radius * sin(theta - theStep) * sin(phi), 1.0f);

                // v0.Color    = Vec4f(0.0f, 1.0f, 0.0f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f(radius * sin(theta - theStep) * cos(phi + phiStep), radius * cos(theta - theStep),
                                    radius * sin(theta - theStep) * sin(phi + phiStep), 1.0f);

                // v0.Color    = Vec4f(0.0f, 0.0f, 1.0f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f(radius * sin(theta) * cos(phi + phiStep), radius * cos(theta),
                                    radius * sin(theta) * sin(phi + phiStep), 1.0f);

                // v0.Color    = Vec4f(1.0f, 1.0f, 0.0f, 0.0f);
                vertices.push_back(v0);

                indices.insert(indices.end(), {index, index + 1, index + 2, index + 2, index + 3, index});

                index += 4;
            }
        }

        return RenderInfo(std::move(vertices), std::move(indices), RenderDevice::MergeMode::COLOR_MODE, 0);
    }
};
class Plane
{
};
class Cone
{
  public:
    static RenderInfo offload(float height, float radius)
    {
        std::vector<Pipeline3D::VertexAttrib3D> vertices;
        std::vector<uint32_t>                   indices;

        constexpr float                         pi    = 3.141592f;
        float                                   hStep = 0.5f;
        float                                   tStep = 0.5f;

        float                                   h     = 0;
        float                                   t     = 0;
        uint32_t                                index = 0;

        while (h < height)
        {
            auto z = height - h;
            for (t = 0; t < 2 * pi; t += tStep)
            {
                Pipeline3D::VertexAttrib3D v0;
                v0.Position = Vec4f(z * cos(t), h, z * sin(t), 1.0f);
                v0.Color    = Vec4f(0.4f, 0.4f, 0.4f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f(z * cos(t + tStep), h, z * sin(t + tStep), 1.0f);
                // v0.Color    = Vec4f(0.0f, 1.0f, 0.0f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f((z - hStep) * cos(t + tStep), h + hStep, (z - hStep) * sin(t + tStep), 1.0f);
                // v0.Color    = Vec4f(0.0f, 0.0f, 1.0f, 0.0f);
                vertices.push_back(v0);
                v0.Position = Vec4f((z - hStep) * cos(t), h + hStep, (z - hStep) * sin(t), 1.0f);
                // v0.Color    = Vec4f(1.0f, 1.0f, 0.0f, 0.0f);
                vertices.push_back(v0);
                indices.insert(indices.end(), {index + 2, index + 1, index, index + 3, index + 2, index});
                index += 4;
            }
            h = h + hStep;
        }
        return RenderInfo(std::move(vertices), std::move(indices), RenderDevice::MergeMode::COLOR_MODE, 0);
    }
};
} // namespace Shape

namespace PhysicsSimulation
{
// Not much but ...

struct Sphere
{
    float radius;
    float t         = 0; // Its the time parameter
    Vec3f center    = {0.0f, 0.0f, 0.0f};
    Vec3f direction = {0.0f, 0.0f, 1.0f}; // Sphere moving direction vector
    float coefficient_of_restitution = 1.0f; 
    Sphere()        = default;
    Sphere(float radius) : radius{radius}
    {
    }

    Vec3f simulate(float dt)
    {
        center = center + dt * direction;
        return center;
    }

    bool check_collision(Sphere const &sphere)
    {
        // Check if sphere collides
        // Check for current frame only
        // calculate the distance between the center of the spheres, if that distance is smaller than the sum of the
        // radius, the sphere collides

        auto distance = (sphere.center - this->center).normSquare();
        if (distance <= (this->radius + sphere.radius) * (this->radius + sphere.radius))
        {
            // They collide
            return true;
        }
        else
            return false;
    }

    void resolve_collision(Sphere &sphere, float dt)
    {
        if (!check_collision(sphere))
            return;
        // if collision occurs resolve it first, reverse time to a point where they barely touch each other
        this->center  = this->center - dt * this->direction;
        sphere.center = sphere.center - dt * sphere.direction;

        // Now change their direction
        // Lets use Newtonian Mechanics here .. will try Lagrangian Physics later on

        // So basically, the direction in which spheres move have 3 components
        // which is the contributing factor of movement of sphere along each co-ordinate axis

        // So, when collision occurs, it is hard to resolve. :*_*: :-_-:
        // Need to use more brain power :D

        // Calculating dot product between their velocity vector, as a starting point sounds good
        // If they collide head on, the dot product of their velocity component would be -1

        // Simplifying further, by considering both sphere have same mass, we have
        // Lets formalize this :
        /*

           let u = (ux, uy, uz), (Ax, Ay, Az) -> center A
           let v = (vx, vy, vz), (Bx, By, Bz) -> center B

           // ah yes, calculate the projection of the vector into the line joining their centers

           let n = (Bx - Ax, By - Ay, Bz - Az) -> Vector joining the center of the two masses
           // Any component of the velocity vector along this line, will simply be exchanged for equal masses

           // Lets implement only this much for now



        */

        // indexing, 0 for this current object and 1 for another
        auto mag0 = this->direction.norm();
        auto mag1 = sphere.direction.norm();

        // Normal component of the vector
        auto norm_vec = (sphere.center - this->center).unit();
        auto norm0    = this->direction.unit().dot(norm_vec);
        auto norm1    = sphere.direction.unit().dot(norm_vec);

        // this->direction  = norm0 * norm_vec * mag1;
        // sphere.direction = norm1 * norm_vec * mag0;

        // And now? what next?
        // Find other components of the vector along each axis?
        // We calculated the normal component of the vector along intersection point, now we might need tangential
        // component of the velocity vector and blend them for direction

        // Tangential component along the line of intersecton
        // I guess theres a plane where vectors are normal to the norm_vec,
        // so shall I find this plane first? or maybe not?
        // let a = (ax,ay,az), then there is a plane such that it is the normal of the plane
        // Ax+By+Cz+D = 0, where (A,B,C) = (ax,ay,az), but there's no point ... we could calculate that point for that
        // purpose, but there should be another way Lmao .. except for some special circumstance, the tangential vector
        // can be found trivially.
        //
        // So at the point of intersection, create a very large tangential circle, find it where it may intersect any
        // co-ordinate axes One is pretty simple : (-ay,ax,0) Others : (ay,-ax,0), (0,az,-ay) .. so on
        //
        auto tan_vec = Vec3f(-norm_vec.y, norm_vec.x, 0.0f).unit();
        // calculate the tangential component of the both velocity vectors along this tangent
        // Since no force is imparted along tangential direction, they remain unchanged
        auto tan0 = this->direction.unit().dot(tan_vec);
        auto tan1 = sphere.direction.unit().dot(tan_vec);

        // This should be enough, I guess.

        /*this->direction  = this->direction + tan0 * mag0 * tan_vec;
        sphere.direction = sphere.direction + tan1 * mag1 * tan_vec;*/

        // Baka mitai
        this->direction  = (norm1 * norm_vec * mag1 + tan0 * mag0 * tan_vec) * this->coefficient_of_restitution;
        sphere.direction = (norm0 * norm_vec * mag0 + tan1 * mag1 * tan_vec) * sphere.coefficient_of_restitution;

        // this->direction  = Vec3f::Cross(this->direction, Vec3f(0.0f, 1.0f, 0.0f));
        // sphere.direction = Vec3f::Cross(sphere.direction, Vec3f(0.0f, 1.0f, 0.0f));
        // After detection, do I need to progress 1 frame further or they basically get stuck?
    }
};

struct Plane;

class PhysicsHandler
{
    // It simulates the Newtonian physics on its member along with othter collision detection and resolution
    // Lets start with sphere bouncing under effect of gravity and the restitution effect

  public:
    std::vector<Sphere>    spheres;
    std::vector<uint32_t>  renderIndex{};

    constexpr static float gravity                   = 9.8f;
    constexpr static float coefficient_of_restituion = 1.0f;

    PhysicsHandler()                                 = default;

    PhysicsHandler(RenderList &renderlist)
    {
        spheres.reserve(2);
        Sphere sph;
        sph.radius    = 0.45f;
        sph.center    = Vec3f(-5.0f, 0.1f, 0.0f);
        sph.direction = Vec3f(1.0f, 15.0f, 1.0f);
        sph.coefficient_of_restitution = 0.6f;
        spheres.push_back(sph);

        // capture the index
        renderlist.AddRenderable(Shape::Sphere::offload(sph.radius, 0.5f, 0.5f,Vec4f(0.0f,0.5f,0.0f,1.0f)));
        renderIndex.push_back(renderlist.Renderables.size()-1);

        sph.radius    = 0.5f;
        sph.center    = Vec3f(4.0f, 0.1f, 0.0f);
        sph.direction = Vec3f(-1.75f, 15.0f, 0.0f);
        sph.coefficient_of_restitution = 0.75f;
        spheres.push_back(sph);
        renderlist.AddRenderable(Shape::Sphere::offload(sph.radius, 0.5f, 0.5f,Vec4f(0.4f,0.0f,0.4f,0.0f)));
        renderIndex.push_back(renderIndex.back() + 1);
    }

    void simulate(float dt, Plane &plane, Sphere &, Sphere&); 
    //{
    //    for (auto &sph : spheres)
    //    {
    //        sph.direction = sph.direction + gravity * dt * Vec3f(0.0f, -1.0f, 0.0f);
    //        sph.center    = sph.center + dt * sph.direction;
    //        sph.
    //    }
    //    // if each sphere collide with the plane, reverse the velocity direction affected by coefficient of restitution
    //}

    void render(RenderList &renderlist)
    {
        // Add the model transform
        // Woahh .. I already want ranges::zip()
        for (size_t i = 0; i < spheres.size(); ++i)
        {
            renderlist.Renderables.at(renderIndex.at(i)).model_transform =
                Mat4f(1.0f).translate(spheres.at(i).center).scale(Vec3f(spheres.at(i).radius));
        }
    }
};

struct Plane
{
    Vec3f coord[4]; // In clockwise ordering for normal calculation

    bool  IntersectAndResolve(Sphere &sphere, float dt)
    {
        // It shouldn't be that hard
        // So a sphere intersects/collide with a plane if the center of the sphere is lesser than radius distance from
        // the plane How to calculate that distance? Vector to the rescue

        /*
            Let P be the plane. Vectorically, if p is the point in the plane and n is the normal vector then,
            P : n.(x-p) = 0, is the plane.
            Just in case, lets parametrize the plane first, to not check if the point is inside the plane boundary

            Starting at vertex v[0], we have
            the plane can be parameterized as,

            v[3]               v[2]




            v[0]               v[1]

            x = v[0] + s ( v[1] - v[0])
            y = v[0] + t ( v[3] - v[0])

        */

        // Lets defer the above calculation, checking only for intersection shouldn't need that to solve

        // Let C be the sphere's center
        //
        // Intersection with plane is checked in multiple steps
        // First the normal distance between sphere center and plane defined by the above co-ordinates
        // Find the normal vector first
        // Since points are taken in clockwise ordering, the normal vector is
        auto normal = Vec3f::Cross(coord[1] - coord[0], coord[2] - coord[1]).unit();
        // take any arbitrary point in the plane and find the plane_constant D
        auto plane_constant = -(normal.dot(coord[3]));
        auto norm_distance  = normal.dot(sphere.center) + plane_constant;
        if (norm_distance > sphere.radius)
            return false;
        // Check if the intersection point really lies inside the plane boundary
        // First approach :
        // Take the sphere's center and add norm_distance*(-normal) to it

        // It will reach to the plane's intersection point
        // Check if that point lies within the plane now
        // If the plane is concave, it needs decomposition which we will be handling later on

        auto intersect_point = sphere.center + -norm_distance * normal;
        // For planar plane, we have, as a special case for efficient collision resolution

        auto a_vec = intersect_point - coord[0];
        auto b_vec = coord[1] - coord[0];
        auto c_vec = coord[3] - coord[0];

        // Check for the linear combination
        // Using Cramers rule to solve for the linear combination, we have
        auto del  = b_vec.x * c_vec.y - c_vec.x * b_vec.y;
        auto del1 = a_vec.x * c_vec.y - a_vec.y * c_vec.x;
        auto del2 = b_vec.x * a_vec.y - a_vec.x * b_vec.y;

        if (del == 0)
        {
            // switch to different linear combination
            del  = b_vec.y * c_vec.z - c_vec.y * b_vec.z;
            del1 = a_vec.y * c_vec.z - c_vec.y * a_vec.z;
            del2 = b_vec.y * a_vec.z - a_vec.y * b_vec.z;
        }
        // if its zero again switch to another linear combination
        if (del == 0)
        {
            del  = b_vec.x * c_vec.z - c_vec.x * b_vec.z;
            del1 = a_vec.x * c_vec.z - c_vec.x * a_vec.z;
            del2 = b_vec.x * a_vec.z - a_vec.x * b_vec.z;
        }

        auto s = del1 / del;
        auto t = del2 / del;

        if ((s >= 0 and s <= 1) or (t >= 0 and t <= 1))
        {
            // Yes it intersects, so resolve the collision
            // As usual, reverse time
            sphere.center = sphere.center - dt * sphere.direction;
            // reflect along the normal direction of the plane
            sphere.direction = sphere.direction.norm() * sphere.direction.reflect(normal).unit() * sphere.coefficient_of_restitution;
            return true;
        }
        return false;
    }
};

void PhysicsHandler::simulate(float dt, Plane &plane, Sphere& sphA, Sphere&sphB)
{
    for (auto &sph : spheres)
    {
        sph.direction = sph.direction + gravity * dt * Vec3f(0.0f, -1.0f, 0.0f);
        sph.center    = sph.center + dt * sph.direction;
        plane.IntersectAndResolve(sph, dt);
        // allow collision with outer spheres too 
        sph.resolve_collision(sphA, dt);
        sph.resolve_collision(sphB, dt);
    }
    // if each sphere collide with the plane, reverse the velocity direction affected by coefficient of restitution
}

} // namespace PhysicsSimulation
