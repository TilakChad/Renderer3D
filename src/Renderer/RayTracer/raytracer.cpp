#include "../../include/raytracer.h"

namespace RayTracer
{
void RenderTriangle(Vec3f const &a, Vec3f const &b, Vec3f const &c)
{
    // I guess there should be a near plane where image will be projected to
    // Eye co-ordinate where the camera/eye will be placed at .. typically origin looking toward -ve z axis -> OpenGL
    // convention And rays, a lot of rays going from eye through each pixels toward scene/objects/world

    // Setting near plane at 0.3f, we have

    float n = 0.2f;
    // Grids equal to size of our screen and each grid equal to each pixel
    // Perspective projection didn't require this much info though .. since most works were done in rasterisation stage

    // So width is width and height is height. And it should be centered at (0,0,-0.3f)..
    // I wonder if its the way to do ray tracing

    auto platform = GetCurrentPlatform();
    auto width    = platform.colorBuffer.width;
    auto height   = platform.colorBuffer.height;
    auto channels = platform.colorBuffer.noChannels;

    // Center of the screen lies exactly at (width/2) and (height/2)...
    // Lets take each pixel size as having size 0.5f x 0.5f;

    int     pixelCount = 0;
    float   delX       = 0.02f;
    float   delY       = 0.02f;

    int32_t midH       = height / 2;
    int32_t midW       = width / 2;
    // At midH, midW, the centre of the screen will lie
    // If midH and midW both are odd, then midH and midW occupies the (0,0) pixel position;
    // Same everywhere

    Plane simplex(a, b, c);
    // The equation of the simplex
    auto pointInsideTri = [=](const Vec3f &point) {
        // Since the point is right on the plane, but not sure if it lies inside the triangle boundary we could use
        // method of half plane Barycentric co-ordinates is too much effort

        auto v0 = b - a;
        auto v1 = c - b;
        auto v2 = a - c;

        // I guess we need plane orientation to decide if two vector are oriented clockwise or anti-clockwise in the
        // given plane
        auto d0 = Vec3f::Cross(v0, point - a);
        auto d1 = Vec3f::Cross(v1, point - b);
        auto d2 = Vec3f::Cross(v2, point - c);

        // Inefficent code maybe
        // We again need to determine if the obtained vectors are anti-parallel or not
        // if vectors are parallel, then their cross product is zero and if they are anti-parallel too
        // if vectors are parallel, their cos(theta) = -1

        // a.b = |a||b|cos(theta)
        if (std::fabs(Vec3f::Dot(d0, simplex.normal) + d0.norm() * simplex.normal.norm()) < 0.0001f)
        {
            if (std::fabs(Vec3f::Dot(d1, simplex.normal) + d1.norm() * simplex.normal.norm()) < 0.001f)
            {
                if (std::fabs(Vec3f::Dot(d2, simplex.normal) + d2.norm() * simplex.normal.norm()) < 0.001f)
                {
                    return true; 
                }
            }
        }
        else if (std::fabs(Vec3f::Dot(d0, simplex.normal) - d0.norm() * simplex.normal.norm()) < 0.0001f)
        {
            if (std::fabs(Vec3f::Dot(d1, simplex.normal) - d1.norm() * simplex.normal.norm()) < 0.001f)
            {
                if (std::fabs(Vec3f::Dot(d2, simplex.normal) - d2.norm() * simplex.normal.norm()) < 0.001f)
                {
                    return true;
                }
            }
        }
        return false;
    };

    for (int32_t h = 0; h < height; ++h)
    {
        int32_t y = (height - 1 - h) - midH;
        for (int32_t w = 0; w < width; ++w)
        {
            int32_t x = w - midW;
           /* if (x == 0 && y == 0)
                __debugbreak();*/
            // x * delX , y * delY is location for pixel through which ray must pass
            // obviously the depth value is near plane and origin is the eye/camera
            Ray  tracer(Vec3f(0.0f, 0.0f, 0.0f), Vec3f(x * delX, y * delY, -n));

            auto point = tracer.Intersect(simplex);
            // if (point is inside triangle)

            if (pointInsideTri(point))
            {
                // std::cout << h << " " << w << '\n';
                // plot the point 
                auto pixel =
                    platform.colorBuffer.buffer + (h * platform.colorBuffer.width + w) * platform.colorBuffer.noChannels;
                pixel[0] = 0xFF;
            }
        }
    }
}
} // namespace RayTracer