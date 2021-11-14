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

    float Norm() const
    {
        return std::sqrt(x * x + y * y);
    }
    float NormSquare() const
    {
        return x * x + y * y;
    }
    constexpr Vec2 operator*(const Vec2 &vec) const
    {
        return Vec2(x * vec.x, y * vec.y);
    }

    T &operator[](size_t index)
    {
        switch (index)
        {
        case 0:
            return x;
        case 1:
            return y;
        default:
            assert(!"Assertion failed vec2 out of range");
        }
    }

    template <cNumeric U> constexpr auto operator*(U scalar) const
    {
        return Vec2<decltype(scalar * x)>{scalar * x, scalar * y};
    }

    constexpr static Vec3<T> Cross(const Vec2 &vec1, const Vec2 &vec2);
};

template <cNumeric T, cNumeric U> inline constexpr auto operator*(U scalar, Vec2<T> const &vec)
{
    return vec * scalar;
}

using Vec2f  = Vec2<float32>;
using Vec2u8 = Vec2<uint8_t>;

template <cNumeric T> struct Vec3
{

    T x, y, z;
    Vec3() = default;
    explicit Vec3(T a) : x{a}, y{a}, z{a}
    {
    }

    explicit Vec3(Vec2<T> xy, T z) : x{xy.x}, y{xy.y}, z{z}
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

    constexpr float norm() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }
    constexpr T normSquare() const
    {
        return x * x + y * y + z * z;
    }

    constexpr Vec3 unit() const 
    {
        float mag = norm(); 
        if (mag == 0)
            return Vec3(); 
        return Vec3(x / mag, y / mag, z / mag);
    }
    template <cNumeric U> constexpr auto operator*(U scalar) const -> Vec3<decltype(scalar * x)>
    {
        return Vec3<decltype(scalar * x)>(x * scalar, y * scalar, z * scalar);
    }

    constexpr Vec3 operator/(T scalar)
    {
        return Vec3(x / scalar, y / scalar, z / scalar);
    }

    constexpr static Vec3 Cross(Vec3 const& vec1, Vec3 const& vec2)
    {
        Vec3 res; 
        res.x = vec1.y * vec2.z - vec1.z * vec2.y; 
        res.y = vec1.z * vec2.x - vec1.x * vec2.z; 
        res.z = vec1.x * vec2.y - vec1.y * vec2.x; 
        return res;
    }

    constexpr static T Dot(Vec3 const &vec1, Vec3 const &vec2)
    {
        return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
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

template <cNumeric T> std::ostream &operator<<(std::ostream &os, Vec3<T> vec)
{
    return os << "Vec3() : x -> " << vec.x << ", y -> " << vec.y << ", z -> " << vec.z << std::endl;
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

template <cNumeric T> struct Vec4
{
    T x, y, z, w; // w is the fourth component
    Vec4() = default;

    explicit Vec4(T a) : x{a}, y{a}, z{a}, w{a}
    {
    }
    explicit Vec4(Vec2<T> xy, T z, T w) : x{xy.x}, y{xy.y}, z{z}, w{w}
    {
    }
    explicit Vec4(Vec3<T> xyz, T w) : x{xyz.x}, y{xyz.y}, z{xyz.z}, w{w}
    {
    }
    Vec4(T x, T y, T z, T w) : x{x}, y{y}, w{w}, z{z}
    {
    }
    Vec4 operator+(const Vec4 &vec) const
    {
        return Vec4(x + vec.x, y + vec.y, z + vec.z, w + vec.w);
    }
    Vec4 operator-(const Vec4 &vec) const
    {
        return Vec4(x - vec.x, y - vec.y, z - vec.z, w - vec.w);
    }
    Vec4 operator*(const Vec4 &vec) const
    {
        return Vec4(x * vec.x, y * vec.y, z * vec.z, w * vec.w);
    }

    Vec4 operator+(T scalar) const
    {
        return Vec4(x + scalar, y + scalar, z + scalar, w + scalar);
    }
    Vec4 operator-(T scalar) const
    {
        return Vec4(x - scalar, y - scalar, z - scalar, w - scalar);
    }
    template <cNumeric U> auto operator*(U scalar) const
    {
        return Vec4<decltype(scalar * x)>(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    Vec4 PerspectiveDivide() const 
    {
        // Don't waste information 
        return Vec4(x / w, y / w, z / w, 1.0f / w);
    }
    T &operator[](size_t index)
    {
        // LOL ..
        switch (index)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        case 3:
            return w;
        default:
            assert("!Vec4 out of range");
        }
    }
    const T &operator[](size_t index) const
    {
        // LOL ..
        switch (index)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        case 3:
            return w;
        default:
            assert("!Vec4 out of range");
        }
    }
};

template <cNumeric U, cNumeric T> auto operator*(U scalar, const Vec4<T> &vec) -> Vec4<decltype(scalar * vec.x)>
{
    return vec.operator*(scalar);
}

using Vec4f  = Vec4<float>;
using Vec4u8 = Vec4<uint8_t>;

template <cNumeric T> 
inline std::ostream &operator<<(std::ostream& os, const Vec4<T>& vec)
{
    return os << "Vec4() -> "
              << "x : " << vec.x << ", y : " << vec.y << ", z : " << vec.z << ", w : " << vec.w << std::endl;
}