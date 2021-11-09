#pragma once

#include "./vec.hpp"

template <cNumeric T> struct Mat4
{
    // Might use row major (row first filling) semantics, Not goint with openGL style
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

    constexpr Mat4 scale(Vec3<T>const& vec)
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
    matrix[2][3]   = (- 2 * far_plane * near_plane) / (far_plane - near_plane);
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
    matrix[2][3]   = (-1* far_plane * near_plane) / (far_plane - near_plane);
    matrix[2][2]   = -far_plane / (far_plane - near_plane);
    matrix[3][3]   = 0;
    return matrix;
}

template <cNumeric T> 
std::ostream &operator<<(std::ostream& os, Mat4<T> const& matrix)
{
    os << "\nMatrix -> \n";
    for (int i = 0;i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            os << '[' << i << ']' << '[' << j << ']' << " -> " << matrix.mat[i][j] << "    |    ";
        }
        os << std::endl;
    }
    return os;
}