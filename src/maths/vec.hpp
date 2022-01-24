#pragma once
#include <cassert>
#include <cmath>
#include <compare>
#include <concepts>
#include <cstdint>
#include <type_traits>

// TODO :: Add feature test macros here
#include <intrin.h>
#include <xmmintrin.h>

#include <iostream>

using float32 = float;
using float64 = double;

template <typename T>
concept cNumeric = std::is_integral_v<T> || std::is_floating_point_v<T>;

template <cNumeric T> struct Vec3;

namespace SIMD
{
struct Vec4ss;
}

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

    constexpr T dot(const Vec3 &vec) const
    {
        return x * vec.x + y * vec.y + z * vec.z;
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

    constexpr static Vec3 Cross(Vec3 const &vec1, Vec3 const &vec2)
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

    constexpr auto project(Vec3 const &vec)
    {
        return Vec3::Dot(*this, vec.unit());
    }

    Vec3 reflect(const Vec3 &normal_) const
    {
        auto normal = normal_.unit();
        return *this - 2.0f * this->dot(normal) * normal;
    }

    Vec3 refract(const Vec3 &normal, float RI) const
    {
        // RI is the ratio of refractive index of source medium to destination medium
        auto n_i = this->dot(normal);

        auto k   = 1.0f - RI * RI * (1.0f - n_i * n_i);
        if (k < 0)
            return Vec3();
        return RI * *this - (RI * n_i + std::sqrt(k)) * normal;
    }

    Vec3 refractTIR(const Vec3 &normal, float RI) const // Takes account into total internal reflection
    {
        // RI is the refractive index of source medium to destination medium
        auto n_i = this->dot(normal);

        // Now how to detect total internal reflection ?
        // Some normal flipping required here ..

        if (n_i < 0.0f)
        {
            n_i = -n_i;
            // It is correct normal
            auto k = 1.0f - RI * RI * (1.0f - n_i * n_i);
            if (k < 0)
                return Vec3();
            return RI * *this - (RI * n_i + std::sqrt(k)) * normal;
        }
        else
        {
            // We got outer normal... flip to make it inside normal
            auto nNormal = normal * -1.0f;
            // Now it is the case of light moving from denser medium to rarer medium
            // Now flip the refractive index and use above formula on it
            RI     = 1.0f / RI;
            auto k = 1.0f - RI * RI * (1.0f - n_i * n_i);
            if (k < 0)
                return Vec3();
            return RI * *this - (RI * n_i + std::sqrt(k)) * nNormal;
            // This should do it
        }
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
    union {

        struct
        {
            T x, y, z, w; // w is the fourth component
        };
        T elem[4];
    };

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
    explicit Vec4(Vec3<T> vec) : x{vec.x}, y{vec.y}, z{vec.z}, w{1.0f}
    {
    }
    Vec4(T x, T y, T z, T w) : x{x}, y{y}, w{w}, z{z}
    {
    }
    Vec4(SIMD::Vec4ss const &vec);
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

    T dot(Vec4 vec) const
    {
        return x * vec.x + y * vec.y + z * vec.z + w * vec.w;
    }
    Vec4 PerspectiveDivide() const
    {
        // Don't waste information
        return Vec4(x / w, y / w, z / w, 1.0f / w);
    }
    T &operator[](size_t index)
    {
        return elem[index];
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

template <cNumeric T> inline std::ostream &operator<<(std::ostream &os, const Vec4<T> &vec)
{
    return os << "Vec4() -> "
              << "x : " << vec.x << ", y : " << vec.y << ", z : " << vec.z << ", w : " << vec.w << std::endl;
}

namespace SIMD
{
struct Vec4ss
{
    // These elements aren't mean to be accessed directly and should only be used for high performance operations
    // So no union of __m128 and float[4] thingy
    __m128 vec;
    Vec4ss(float x, float y, float z, float w)
    {
        vec = _mm_set_ps(x, y, z, w);
    }
    Vec4ss(float x)
    {
        vec = _mm_set_ps1(x);
    }
    Vec4ss() = default;
    Vec4ss(__m128 elem) : vec{elem}
    {
    }

    Vec4ss(const Vec4f &vecf) : vec{_mm_set_ps(vecf.x, vecf.y, vecf.z, vecf.w)}
    {
    }
    // Dot product of two vector float vectors
    float dot(const Vec4ss &vec) const
    {
        __m128 res = _mm_dp_ps(this->vec, vec.vec, 0xE1);
        return _mm_cvtss_f32(res);
    }
    Vec4ss operator+(const Vec4ss &vec) const
    {
        return Vec4ss(_mm_add_ps(this->vec, vec.vec));
    }
    Vec4ss operator-(const Vec4ss &vec) const
    {
        return Vec4ss(_mm_sub_ps(this->vec, vec.vec));
    }
    // Element wise multiplication of vector types
    Vec4ss operator*(const Vec4ss &vec1) const
    {
        return Vec4ss(_mm_mul_ps(this->vec, vec1.vec));
    }

    Vec4ss operator*(const float &val) const
    {
        return Vec4ss(_mm_mul_ps(_mm_set_ps1(val), this->vec));
    }

    int compare_f3_greater_eq_than_zero() const 
    {
        return (_mm_movemask_ps(_mm_cmpge_ps(vec, _mm_setzero_ps())) & 0x0F);
    }

    int compare_f3_lesser_eq_than_zero() const 
    {
        return (_mm_movemask_ps(_mm_cmple_ps(vec, _mm_setzero_ps())) & 0x0F);
    }

    Vec4ss swizzle_for_barycentric() const
    {
        return Vec4ss(_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2, 1, 3, 0)));
    }

    static int generate_mask(Vec4ss const& a, Vec4ss const& b, Vec4ss const& c)
    {
        auto a_m = a.compare_f3_greater_eq_than_zero(); 
        auto b_m = b.compare_f3_greater_eq_than_zero(); 
        auto c_m = c.compare_f3_greater_eq_than_zero(); 
        return a_m & b_m & c_m;
    }

    static int generate_neg_mask(Vec4ss const& a, Vec4ss const& b, Vec4ss const& c)
    {
        auto a_m = a.compare_f3_lesser_eq_than_zero();
        auto b_m = b.compare_f3_lesser_eq_than_zero();
        auto c_m = c.compare_f3_lesser_eq_than_zero();
        return a_m & b_m & c_m;
    }
};
} // namespace SIMD

template <cNumeric T> 
Vec4<T>::Vec4(SIMD::Vec4ss const& vec)
{
    float a[4]; 
    _mm_store_ps(a, vec.vec);
    x = a[3]; 
    y = a[2]; 
    z = a[1]; 
    w = a[0];
}