#pragma once
#include <cassert>
#include <cmath>
#include <compare>
#include <concepts>
#include <cstdint>
#include <type_traits>

#include <iostream>

using float32 = float;
using float64 = double;

template <typename T>
concept cNumeric = std::is_integral_v<T> || std::is_floating_point_v<T>;

template <cNumeric T> struct Vec3;

template <cNumeric T> struct Vec2
{
    // No undefined behaviours with union thingy
    T x, y;

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
    Vec2 operator-(T scalar) const
    {
        return {x - scalar, y - scalar};
    }

    Vec2 operator+(const Vec2 &vec) const
    {
        return Vec2(x + vec.x, y + vec.y);
    }
    Vec2 operator+(T scalar) const
    {
        return Vec2(x + scalar, y + scalar);
    }

    static T Determinant(const Vec2 &a, const Vec2 &b)
    {
        return a.x * b.y - b.x * a.y;
    }

    static T Dot(const Vec2 &vec1, const Vec2 &vec2)
    {
        return vec1.x * vec2.x + vec1.y * vec2.y;
    }

    constexpr Vec2 operator*(const Vec2 &vec) const
    {
        return Vec2(x * vec.x, y * vec.y);
    }

    constexpr Vec2 operator*(T scalar) const
    {
        return {scalar * x, scalar * y};
    }

    constexpr static Vec3<T> Cross(const Vec2 &vec1, const Vec2 &vec2);
};

using Vec2f = Vec2<float32>;

template <cNumeric T> struct Vec3
{

    T x, y, z;
    Vec3() = default;
    explicit Vec3(T a) : x{a}, y{a}, z{a}
    {
    }
    Vec3(T x, T y, T z) : x{x}, y{y}, z{z}
    {
    }

    constexpr auto operator<=>(const Vec3 &) const = default;

    constexpr Vec3 operator+(const Vec3 &vec) const
    {
        return Vec3(x + vec.x, y + vec.y, z + vec.z);
    }
    constexpr Vec3 operator-(const Vec3 &vec) const
    {
        return Vec3(x - vec.x, y - vec.y, z - vec.z);
    }

    constexpr Vec3 operator*(const Vec3 &vec) const
    {
        return Vec3(x * vec.x, y * vec.y, z * vec.z);
    }

    constexpr T norm() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }
    constexpr T normSquare() const
    {
        return x * x + y * y + z * z;
    }

    template <cNumeric U> constexpr auto operator*(U scalar) const -> Vec3<decltype(scalar * x)>
    {
        return Vec3<decltype(scalar * x)>(x * scalar, y * scalar, z * scalar);
    }

    constexpr Vec3 operator/(T scalar)
    {
        return Vec3(x / scalar, y / scalar, z / scalar);
    }
};

using Vec3f  = Vec3<float32>;
using Vec3u8 = Vec3<uint8_t>;

template <cNumeric T, cNumeric U> inline constexpr auto operator*(U scalar, const Vec3<T> &vec)
{
    return vec.operator*(scalar);
}

template <cNumeric T> inline constexpr Vec3<T> Vec2<T>::Cross(const Vec2<T> &vec1, const Vec2<T> &vec2)
{
    return Vec3(T{}, T{}, vec1.x * vec2.y - vec2.x * vec1.y);
}

template <cNumeric T>
inline constexpr Vec2<float> LineIntersect(Vec2<T> vec1i, Vec2<T> vec1f, Vec2<T> vec2i, Vec2<T> vec2f)
{
    // It will return parameters u and v, not the actual points of the intersection
    auto d = Vec2<T>::Determinant(vec1f - vec1i, vec2f - vec2i);
    assert(d != 0);

    auto u = Vec2<T>::Determinant(vec2i - vec1i, vec2f - vec2i);
    auto v = Vec2<T>::Determinant(vec1f - vec1i, vec2i - vec1i);
    return Vec2<float>(static_cast<float>(u) / d, static_cast<float>(-v) / d);
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

template <cNumeric T> std::ostream &operator<<(std::ostream &os, Vec2<T> vec)
{
    return os << "x -> " << vec.x << ", y -> " << vec.y << std::endl;
}
