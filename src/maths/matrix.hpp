#pragma once

// Yay .. going to use SIMD
#include <xmmintrin.h>

#include "./vec.hpp"

template <cNumeric T> struct Mat4
{
    // Might use row major (row first filling) semantics, Not going with openGL style
    T mat[4][4]      = {};

    constexpr Mat4() = default;

    // Fills the diagonal
    constexpr Mat4(T fill)
    {
        for (int i = 0; i < 4; ++i)
            mat[i][i] = fill;
    }

    const T *operator[](size_t index) const
    {
        assert(index < 4);
        return mat[index];
    }

    T *operator[](size_t index)
    {
        // Can't control the second semantic though
        assert(index < 4);
        return mat[index];
    }

    constexpr Mat4 translate(const Vec3<T> &vec) const
    {
        Mat4 matrix(1.0f);
        matrix[0][3] = vec.x;
        matrix[1][3] = vec.y;
        matrix[2][3] = vec.z;
        return this->operator*(matrix);
    }

    constexpr Mat4 operator*(const Mat4 &amatrix) const
    {
        Mat4 matrix;
        // Cache efficient matrix multiplication
        // Compiler may auto vectorize it though .. Compilers know a lot of things
        T var = {};
        for (uint8_t row = 0; row < 4; ++row)
        {
            for (uint8_t k = 0; k < 4; ++k)
            {
                var = mat[row][k];
                for (uint8_t col = 0; col < 4; ++col)
                    matrix[row][col] += var * amatrix[k][col];
            }
        }
        return matrix;
    }

    // constexpr Mat4 operator*(const Mat4 &amatrix) const
    //{
    //    Mat4 matrix;
    //    // Cache efficient matrix multiplication
    //    // Compiler may auto vectorize it
    //    T var = {};
    //    for (uint8_t row = 0; row < 4; ++row)
    //    {
    //        for (uint8_t col = 0; col < 4; ++col)
    //        {
    //            T var = {};
    //            for (uint8_t k = 0; k < 4; ++k)
    //                var += mat[row][k]*amatrix[k][col];
    //            matrix[row][col] = var;
    //        }
    //    }
    //    return matrix;
    //}

    constexpr Mat4 rotateX(float angle)
    {
        Mat4 matrix(1);
        matrix[1][1] = std::cos(angle);
        matrix[2][1] = std::sin(angle);
        matrix[1][2] = -matrix[2][1];
        matrix[2][2] = matrix[1][1];
        return this->operator*(matrix);
    }

    constexpr Mat4 rotateY(float angle)
    {
        Mat4 matrix(1);
        matrix[0][0] = std::cos(angle);
        matrix[2][0] = -std::sin(angle);
        matrix[0][2] = -matrix[2][0];
        matrix[2][2] = matrix[0][0];
        return *this * matrix;
    }

    constexpr Mat4 rotateZ(float angle)
    {
        Mat4 matrix(1);
        matrix[0][0] = std::cos(angle);
        matrix[1][0] = std::sin(angle);
        matrix[0][1] = -matrix[1][0];
        matrix[1][1] = matrix[0][0];
        return *this * matrix;
    }

    constexpr Mat4 scale(Vec3<T> const &vec)
    {
        Mat4 matrix(1);
        matrix[0][0] = vec.x;
        matrix[1][1] = vec.y;
        matrix[2][2] = vec.z;
        return *this * matrix;
    }

    constexpr Vec4<T> operator*(const Vec4<T> &avec) const
    {
        Vec4<T> vec(T{});
        T       var = {};
        for (uint8_t row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
                vec[row] += avec[col] * mat[row][col];
        }
        return vec;
    }
};

inline Mat4<float> OrthoProjection(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Mat4<float> matrix(1.0f);
    matrix[0][0] = 2.0f / (right - left);
    matrix[1][1] = 2.0f / (top - bottom);
    matrix[2][2] = -2.0f / (zFar - zNear);

    matrix[0][3] = (right + left) / (left - right);
    matrix[1][3] = (top + bottom) / (bottom - top);
    matrix[2][3] = (zFar + zNear) / (zNear - zFar);
    return matrix;
}

using Mat4f = Mat4<float>;

inline Mat4f Perspective(float aspect_ratio, float fovy = 1.0f / 3 * 3.141592, float near_plane = 0.1f,
                         float far_plane = 10.0f)
{
    Mat4f matrix(1.0f);
    float tan_fovy = std::sin(fovy) / std::cos(fovy);
    float top      = near_plane * tan_fovy;
    float right    = aspect_ratio * top;

    matrix[0][0]   = near_plane / right;
    matrix[1][1]   = near_plane / top;
    matrix[3][2]   = -1;
    matrix[2][3]   = (-2 * far_plane * near_plane) / (far_plane - near_plane);
    matrix[2][2]   = -(far_plane + near_plane) / (far_plane - near_plane);
    matrix[3][3]   = 0;
    return matrix;
}

inline Mat4f PerspectiveAlt(float aspect_ratio, float fovy = 1.0f / 3 * 3.141592, float near_plane = 0.1f,
                            float far_plane = 10.0f)
{
    Mat4f matrix(1.0f);
    float tan_fovy = std::sin(fovy) / std::cos(fovy);
    float top      = near_plane * tan_fovy;
    float right    = aspect_ratio * top;

    matrix[0][0]   = near_plane / right;
    matrix[1][1]   = near_plane / top;
    matrix[3][2]   = -1;
    matrix[2][3]   = (-1 * far_plane * near_plane) / (far_plane - near_plane);
    matrix[2][2]   = -far_plane / (far_plane - near_plane);
    matrix[3][3]   = 0;
    return matrix;
}

template <cNumeric T> std::ostream &operator<<(std::ostream &os, Mat4<T> const &matrix)
{
    os << "\nMatrix -> \n";
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            os << '[' << i << ']' << '[' << j << ']' << " -> " << matrix.mat[i][j] << "    |    ";
        }
        os << std::endl;
    }
    return os;
}

// Determinant of matrices
// Should I write a linear algebra math functions here??

inline Mat4f lookAtMatrix(const Vec3f &cameraPos, const Vec3f &target, const Vec3f &up)
{
    auto forward_vec = (cameraPos - target).unit();
    auto right       = Vec3f::Cross(up, forward_vec).unit();

    auto now_up      = Vec3f::Cross(forward_vec, right).unit();

    Mat4 matrix(1.0f);
    matrix[0][0] = right.x;
    matrix[0][1] = right.y;
    matrix[0][2] = right.z;
    matrix[0][3] = -cameraPos.dot(right);

    matrix[1][0] = now_up.x;
    matrix[1][1] = now_up.y;
    matrix[1][2] = now_up.z;

    matrix[1][3] = -cameraPos.dot(now_up);

    matrix[2][0] = forward_vec.x;
    matrix[2][1] = forward_vec.y;
    matrix[2][2] = forward_vec.z;
    matrix[2][3] = -cameraPos.dot(forward_vec);
    return matrix;
}

namespace SIMD
{

struct Mat4ss
{
    __m128 mat[4];
    Mat4ss()
    {
        /*    for (int i = 0; i < 4; ++i)
                mat[i] = _mm_setzero_ps();*/
    }

    Mat4ss(Mat4f const &m)
    {
        mat[0] = _mm_set_ps(m.mat[0][0], m.mat[0][1], m.mat[0][2], m.mat[0][3]);
        mat[1] = _mm_set_ps(m.mat[1][0], m.mat[1][1], m.mat[1][2], m.mat[1][3]);
        mat[2] = _mm_set_ps(m.mat[2][0], m.mat[2][1], m.mat[2][2], m.mat[2][3]);
        mat[3] = _mm_set_ps(m.mat[3][0], m.mat[3][1], m.mat[3][2], m.mat[3][3]);
    }

    Mat4ss(float s)
    {
        mat[0] = _mm_set_ps(s, 0.0f, 0.0f, 0.0f);
        mat[1] = _mm_set_ps(0.0f, s, 0.0f, 0.0f);
        mat[2] = _mm_set_ps(0.0f, 0.0f, s, 0.0f);
        mat[3] = _mm_set_ps(0.0f, 0.0f, 0.0f, s);
    }

    Vec4ss mul_dp(Vec4ss const &vec) const
    {

        float ans[4];
        for (int i = 0; i < 4; ++i)
        {
            ans[i] = _mm_cvtss_f32(_mm_dp_ps(mat[i], vec.vec, 0xF1));
        }
        return Vec4ss(ans[0], ans[1], ans[2], ans[3]);
    }

    Vec4ss mul_hadd(Vec4ss const &vec) const
    {
        auto a = _mm_mul_ps(this->mat[0], vec.vec);
        auto b = _mm_mul_ps(this->mat[1], vec.vec);
        auto c = _mm_mul_ps(this->mat[2], vec.vec);
        auto d = _mm_mul_ps(this->mat[3], vec.vec);
        return Vec4ss(_mm_hadd_ps(_mm_hadd_ps(d, c), _mm_hadd_ps(b, a)));
    }

    Vec4ss operator*(Vec4ss const &vec) const
    {
        // now write SIMD matrix multiplication here .. lmao
        // Now remains matrix multiplication
        // Around 5 times faster than usual mat4f * vec4f

        // Older version
        /*
        float ans[4];
        for (int i = 0; i < 4; ++i) {
            ans[i] = _mm_cvtss_f32(_mm_dp_ps(mat[i], vec.vec, 0xF1));
        }
        return Vec4ss(ans[0], ans[1], ans[2], ans[3]);
        */

        // There's this rumor that dot product is actually slow on modern processors
        // So lets try writing it in a different way
        // This apparently is a little faster
        auto a = _mm_mul_ps(this->mat[0], vec.vec);
        auto b = _mm_mul_ps(this->mat[1], vec.vec);
        auto c = _mm_mul_ps(this->mat[2], vec.vec);
        auto d = _mm_mul_ps(this->mat[3], vec.vec);
        return Vec4ss(_mm_hadd_ps(_mm_hadd_ps(d, c), _mm_hadd_ps(b, a)));
    }

    Vec4f operator*(Vec4f const &ivec) const
    {
        // now write SIMD matrix multiplication here .. lmao
        // Now remains matrix multiplication
        // Around 5 times faster than usual mat4f * vec4f

        // Older version
        /*
        float ans[4];
        for (int i = 0; i < 4; ++i) {
            ans[i] = _mm_cvtss_f32(_mm_dp_ps(mat[i], vec.vec, 0xF1));
        }
        return Vec4ss(ans[0], ans[1], ans[2], ans[3]);
        */

        // There's this rumor that dot product is actually slow on modern processors
        // So lets try writing it in a different way
        // This apparently is a little faster
        Vec4ss vec = Vec4ss(ivec);
        auto a = _mm_mul_ps(this->mat[0], vec.vec);
        auto b = _mm_mul_ps(this->mat[1], vec.vec);
        auto c = _mm_mul_ps(this->mat[2], vec.vec);
        auto d = _mm_mul_ps(this->mat[3], vec.vec);
        vec = Vec4ss(_mm_hadd_ps(_mm_hadd_ps(d, c), _mm_hadd_ps(b, a)));
        return Vec4f(vec);
    }

    // This around 3-4 times faster
    // Optimized ones 7-8 times
    Mat4ss matrixmul_dp(Mat4ss const &vec) const
    {
        Mat4ss result;
        // Transposed using SIMD
        // maybe start with a transpose, since that sounds good
        Mat4ss transposed = vec.transpose();
        // hmm... its quite simple compared to what i have thought

        // let compiler unroll the loop
        auto dot = [](auto x, auto y) { return _mm_cvtss_f32(_mm_dp_ps(x, y, 0xf1)); };
        for (int i = 0; i < 4; ++i)
        {
            result.mat[i] = _mm_set_ps(dot(mat[i], transposed.mat[0]), dot(mat[i], transposed.mat[1]),
                                       dot(mat[i], transposed.mat[2]), dot(mat[i], transposed.mat[3]));
        }
        return result;
    }
    Mat4ss matrixmul_ps(Mat4ss const &vec) const
    {
        Mat4ss result;
        __m128 a, b, c, d, r0, r1, r2, r3;
        float  elems[4][4];
        _mm_store_ps(elems[0], mat[0]);
        _mm_store_ps(elems[1], mat[1]);
        _mm_store_ps(elems[2], mat[2]);
        _mm_store_ps(elems[3], mat[3]);
        for (int i = 0; i < 4; ++i)
        {
            a             = _mm_set_ps1(elems[i][3]);
            b             = _mm_set_ps1(elems[i][2]);
            c             = _mm_set_ps1(elems[i][1]);
            d             = _mm_set_ps1(elems[i][0]);
            r0            = _mm_mul_ps(a, vec.mat[0]);
            r1            = _mm_mul_ps(b, vec.mat[1]);
            r2            = _mm_mul_ps(c, vec.mat[2]);
            r3            = _mm_mul_ps(d, vec.mat[3]);

            result.mat[i] = _mm_add_ps(_mm_add_ps(r0, r1), _mm_add_ps(r2, r3));
        }

        // Lets extract all the required parameters first here
        return result;
    }

    Mat4ss operator*(Mat4ss const &vec) const
    {
        // Core operation .. better write a good simd thinking nicely
        Mat4ss result;
        //// Transposed using SIMD
        //// maybe start with a transpose, since that sounds good
        // Mat4ss transposed = vec.transpose();
        //// hmm... its quite simple compared to what i have thought

        //// let compiler unroll the loop
        // auto dot = [](auto x, auto y) { return _mm_cvtss_f32(_mm_dp_ps(x, y, 0xf1)); };
        // for (int i = 0; i < 4; ++i) {
        //	result.mat[i] = _mm_set_ps(
        //		dot(mat[i], transposed.mat[0]),
        //		dot(mat[i], transposed.mat[1]),
        //		dot(mat[i], transposed.mat[2]),
        //		dot(mat[i], transposed.mat[3])
        //	);
        // }
        //  Hmm... really ? This simple?
        //  Lets do some benchmarking now .. First normal check
        //  TODO :: Write this without using transpose of another matrix
        //  Use broadcast functions here
        __m128 a, b, c, d, r0, r1, r2, r3;
        float  elems[4][4];
        _mm_store_ps(elems[0], mat[0]);
        _mm_store_ps(elems[1], mat[1]);
        _mm_store_ps(elems[2], mat[2]);
        _mm_store_ps(elems[3], mat[3]);
        for (int i = 0; i < 4; ++i)
        {
            a             = _mm_set_ps1(elems[i][3]);
            b             = _mm_set_ps1(elems[i][2]);
            c             = _mm_set_ps1(elems[i][1]);
            d             = _mm_set_ps1(elems[i][0]);
            r0            = _mm_mul_ps(a, vec.mat[0]);
            r1            = _mm_mul_ps(b, vec.mat[1]);
            r2            = _mm_mul_ps(c, vec.mat[2]);
            r3            = _mm_mul_ps(d, vec.mat[3]);

            result.mat[i] = _mm_add_ps(_mm_add_ps(r0, r1), _mm_add_ps(r2, r3));
        }

        // Lets extract all the required parameters first here
        return result;
    }

    // void show()
    //{
    //     for (int i = 0; i < 4; ++i)
    //     {
    //         Vec4ss(mat[i]).show_hori();
    //     }
    // }

    // float sum_first_row()
    //{
    //     return Vec4ss(mat[0]).vector_sum();
    // }

    Mat4ss transpose() const
    {
        Mat4ss mat;
        // Nice .. this version is almost twice faster
        // matrix vector multiplication was way faster with simd
        // Lets try shuffling just first two rows
        __m128 a   = _mm_unpackhi_ps(this->mat[1], this->mat[0]);
        __m128 b   = _mm_unpackhi_ps(this->mat[3], this->mat[2]);
        __m128 c   = _mm_unpacklo_ps(this->mat[1], this->mat[0]);
        __m128 d   = _mm_unpacklo_ps(this->mat[3], this->mat[2]);

        mat.mat[0] = _mm_movehl_ps(a, b);
        mat.mat[1] = _mm_movelh_ps(b, a);

        mat.mat[2] = _mm_movehl_ps(c, d);
        mat.mat[3] = _mm_movelh_ps(d, c);
        // _MM_TRANSPOSE4_PS(mat.mat[0], mat.mat[1], mat.mat[2], mat.mat[3]);
        return mat;
    }

    Mat4ss translate(const Vec3f &vec) const
    {
        Mat4ss temp;
        temp.mat[0] = _mm_set_ps(1.0f, 0.0f, 0.0f, vec.x);
        temp.mat[1] = _mm_set_ps(1.0f, 0.0f, 0.0f, vec.y);
        temp.mat[2] = _mm_set_ps(1.0f, 0.0f, 0.0f, vec.z);
        temp.mat[3] = _mm_set_ps(1.0f, 0.0f, 0.0f, 1.0f);
        return this->operator*(temp);
    }

    Mat4ss rotateY(float angle)
    {
        Mat4ss matrix;
        auto   c = std::cos(angle), s = std::sin(angle);

        // matrix[0][0]  = std::cos(angle);
        // matrix[2][0]  = -std::sin(angle);
        // matrix[0][2]  = -matrix[2][0];
        // matrix[2][2]  = matrix[0][0];

        matrix.mat[0] = _mm_set_ps(c, 0.0f, s, 0.0f);
        matrix.mat[1] = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f);
        matrix.mat[2] = _mm_set_ps(-s, 0.0f, c, 0.0f);
        matrix.mat[3] = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
        return this->operator*(matrix);
    }
};

inline Mat4ss lookAtMatrix(const Vec3f &cameraPos, const Vec3f &target, const Vec3f &up)
{
    auto forward_vec = (cameraPos - target).unit();
    auto right       = Vec3f::Cross(up, forward_vec).unit();
    auto now_up      = Vec3f::Cross(forward_vec, right).unit();

    // Mat4 matrix(1.0f);
    // matrix[0][0] = right.x;
    // matrix[0][1] = right.y;
    // matrix[0][2] = right.z;
    // matrix[0][3] = -cameraPos.dot(right);

    // matrix[1][0] = now_up.x;
    // matrix[1][1] = now_up.y;
    // matrix[1][2] = now_up.z;

    // matrix[1][3] = -cameraPos.dot(now_up);

    // matrix[2][0] = forward_vec.x;
    // matrix[2][1] = forward_vec.y;
    // matrix[2][2] = forward_vec.z;
    // matrix[2][3] = -cameraPos.dot(forward_vec);

    Mat4ss matrix;
    matrix.mat[0] = _mm_set_ps(right.x, right.y, right.z, -cameraPos.dot(right));
    matrix.mat[1] = _mm_set_ps(now_up.x, now_up.y, now_up.z, -cameraPos.dot(now_up));
    matrix.mat[2] = _mm_set_ps(forward_vec.x, forward_vec.y, forward_vec.z, -cameraPos.dot(forward_vec));
    matrix.mat[3] = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
    return matrix;
}

inline Mat4ss Perspective(float aspect_ratio, float fovy = 1.0f / 3 * 3.141592, float near_plane = 0.1f,
                          float far_plane = 10.0f)
{
    Mat4ss matrix;
    float  tan_fovy = std::sin(fovy) / std::cos(fovy);
    float  top      = near_plane * tan_fovy;
    float  right    = aspect_ratio * top;

    // matrix[0][0]   = near_plane / right;
    // matrix[1][1]   = near_plane / top;
    // matrix[3][2]   = -1;
    // matrix[2][3]   = (-2 * far_plane * near_plane) / (far_plane - near_plane);
    // matrix[2][2]   = -(far_plane + near_plane) / (far_plane - near_plane);
    // matrix[3][3]   = 0;

    matrix.mat[0] = _mm_set_ps(near_plane / right, 0.0f, 0.0f, 0.0f);
    matrix.mat[1] = _mm_set_ps(0.0f, near_plane / top, 0.0f, 0.0f);
    matrix.mat[2] = _mm_set_ps(0.0f, 0.0f, -(far_plane + near_plane) / (far_plane - near_plane),
                               (-2 * far_plane * near_plane) / (far_plane - near_plane));
    matrix.mat[3] = _mm_set_ps(0.0f, 0.0f, -1.0f, 0.0f);
    return matrix;
}
} // namespace SIMD