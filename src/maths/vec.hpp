#pragma once
#include <concepts>
#include <cstdint>
#include <type_traits>
#include <compare>

#include <iostream>

using float32 = float;
using float64 = double;

template <typename T>
concept cNumeric = std::is_integral_v<T> || std::is_floating_point_v<T>;

template <cNumeric T>
struct Vec3;

template <cNumeric T> 
struct Vec2
{
    // No undefined behaviours with union thingy 
    struct
    {
        T x,y;
    };
    Vec2() = default;
    constexpr Vec2(T x) : x{x}, y{y}
    {
    }
    constexpr Vec2(T x, T y) : x{x}, y{y}
    {
    }
    constexpr auto operator<=>(const Vec2 &vec) const = default;

    Vec2           operator-(const Vec2 &vec) const
    {
        return Vec2(x - vec.x, y - vec.y);
    }
    Vec2 operator- (T scalar) const
    {
        return {x-scalar,y-scalar};
    }

    Vec2 operator+(const Vec2 &vec) const
    {
        return Vec2(x + vec.x, y + vec.y);
    }
    Vec2 operator+(T scalar) const
    {
        return Vec2(x+scalar,y+scalar);
    }

    static T Determinant(const Vec2 &a, const Vec2 &b)
    {
        return a.x * b.y - b.x * a.y;
    }

    static T Dot(const Vec2& vec1, const Vec2& vec2) 
    {
        return vec1.x * vec2.x + vec1.y * vec2.y; 
    }

    constexpr Vec2 operator*(const Vec2& vec) const
    {
        return Vec2(x * vec.x, y * vec.y);
    }

    constexpr Vec2 operator*(T scalar) const
    {
        return {scalar*x,scalar*y};
    }

    constexpr static Vec3<T> Cross(const Vec2& vec1, const Vec2& vec2);
};

template <cNumeric T>
struct Vec3
{
    struct
    {
        T x,y,z;
    };
    Vec3() = default; 
    Vec3(T x, T y, T z)
        : x{ x }, y{ y }, z{ z }
    {

    }
};


template<cNumeric T>
inline constexpr Vec3<T> Vec2<T>::Cross(const Vec2<T> & vec1, const Vec2<T> & vec2)
{
    return Vec3(T{},T{},vec1.x*vec2.y - vec2.x * vec1.y);
}

template <cNumeric T> 
inline constexpr Vec2<float> LineIntersect(Vec2<T> vec1i, Vec2<T> vec1f, Vec2<T> vec2i, Vec2<T> vec2f)
{
    // It will return parameters u and v, not the actual points of the intersection 
    auto d = Vec2<T>::Determinant(vec1f-vec1i, vec2f-vec2i);
    assert(d != 0);

    auto u = Vec2<T>::Determinant(vec2i - vec1i, vec2f - vec2i);
    auto v = Vec2<T>::Determinant(vec1f - vec1i, vec2i - vec1i);
    return Vec2<float>(static_cast<float>(u)/d,static_cast<float>(-v)/d);
}

template <typename T>
requires requires(T x)
{
    std::cout << x; 
}
inline void printSomething(T value)
{
    std::cout << "Printed value is " << value << std::endl;
}

template <cNumeric T> std::ostream &operator<<(std::ostream& os, Vec2<T> vec)
{
    return os << "x -> " << vec.x << ", y -> " << vec.y << std::endl;
}

