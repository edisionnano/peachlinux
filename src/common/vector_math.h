// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

// Copyright 2014 Tony Wasserka
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the owner nor the names of its contributors may
//       be used to endorse or promote products derived from this software
//       without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <cmath>
#include <type_traits>

namespace Common {

template <typename T>
class Vec2;
template <typename T>
class Vec3;
template <typename T>
class Vec4;

template <typename T>
class Vec2 {
public:
    T x{};
    T y{};

    constexpr Vec2() = default;
    constexpr Vec2(const T& x_, const T& y_) : x(x_), y(y_) {}

    template <typename T2>
    constexpr Vec2<T2> Cast() const {
        return Vec2<T2>(static_cast<T2>(x), static_cast<T2>(y));
    }

    static constexpr Vec2 AssignToAll(const T& f) {
        return Vec2{f, f};
    }

    constexpr Vec2<decltype(T{} + T{})> operator+(const Vec2& other) const {
        return {x + other.x, y + other.y};
    }
    constexpr Vec2& operator+=(const Vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    constexpr Vec2<decltype(T{} - T{})> operator-(const Vec2& other) const {
        return {x - other.x, y - other.y};
    }
    constexpr Vec2& operator-=(const Vec2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    template <typename U = T>
    constexpr Vec2<std::enable_if_t<std::is_signed_v<U>, U>> operator-() const {
        return {-x, -y};
    }
    constexpr Vec2<decltype(T{} * T{})> operator*(const Vec2& other) const {
        return {x * other.x, y * other.y};
    }

    template <typename V>
    constexpr Vec2<decltype(T{} * V{})> operator*(const V& f) const {
        return {x * f, y * f};
    }

    template <typename V>
    constexpr Vec2& operator*=(const V& f) {
        *this = *this * f;
        return *this;
    }

    template <typename V>
    constexpr Vec2<decltype(T{} / V{})> operator/(const V& f) const {
        return {x / f, y / f};
    }

    template <typename V>
    constexpr Vec2& operator/=(const V& f) {
        *this = *this / f;
        return *this;
    }

    constexpr T Length2() const {
        return x * x + y * y;
    }

    // Only implemented for T=float
    float Length() const;
    float Normalize(); // returns the previous length, which is often useful

    constexpr T& operator[](std::size_t i) {
        return *((&x) + i);
    }
    constexpr const T& operator[](std::size_t i) const {
        return *((&x) + i);
    }

    constexpr void SetZero() {
        x = 0;
        y = 0;
    }

    // Common aliases: UV (texel coordinates), ST (texture coordinates)
    constexpr T& u() {
        return x;
    }
    constexpr T& v() {
        return y;
    }
    constexpr T& s() {
        return x;
    }
    constexpr T& t() {
        return y;
    }

    constexpr const T& u() const {
        return x;
    }
    constexpr const T& v() const {
        return y;
    }
    constexpr const T& s() const {
        return x;
    }
    constexpr const T& t() const {
        return y;
    }

    // swizzlers - create a subvector of specific components
    constexpr Vec2 yx() const {
        return Vec2(y, x);
    }
    constexpr Vec2 vu() const {
        return Vec2(y, x);
    }
    constexpr Vec2 ts() const {
        return Vec2(y, x);
    }
};

template <typename T, typename V>
constexpr Vec2<T> operator*(const V& f, const Vec2<T>& vec) {
    return Vec2<T>(f * vec.x, f * vec.y);
}

using Vec2f = Vec2<float>;

template <>
inline float Vec2<float>::Length() const {
    return std::sqrt(x * x + y * y);
}

template <>
inline float Vec2<float>::Normalize() {
    float length = Length();
    *this /= length;
    return length;
}

template <typename T>
class Vec3 {
public:
    T x{};
    T y{};
    T z{};

    constexpr Vec3() = default;
    constexpr Vec3(const T& x_, const T& y_, const T& z_) : x(x_), y(y_), z(z_) {}

    template <typename T2>
    constexpr Vec3<T2> Cast() const {
        return Vec3<T2>(static_cast<T2>(x), static_cast<T2>(y), static_cast<T2>(z));
    }

    static constexpr Vec3 AssignToAll(const T& f) {
        return Vec3(f, f, f);
    }

    constexpr Vec3<decltype(T{} + T{})> operator+(const Vec3& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }

    constexpr Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    constexpr Vec3<decltype(T{} - T{})> operator-(const Vec3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    constexpr Vec3& operator-=(const Vec3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    template <typename U = T>
    constexpr Vec3<std::enable_if_t<std::is_signed_v<U>, U>> operator-() const {
        return {-x, -y, -z};
    }

    constexpr Vec3<decltype(T{} * T{})> operator*(const Vec3& other) const {
        return {x * other.x, y * other.y, z * other.z};
    }

    template <typename V>
    constexpr Vec3<decltype(T{} * V{})> operator*(const V& f) const {
        return {x * f, y * f, z * f};
    }

    template <typename V>
    constexpr Vec3& operator*=(const V& f) {
        *this = *this * f;
        return *this;
    }
    template <typename V>
    constexpr Vec3<decltype(T{} / V{})> operator/(const V& f) const {
        return {x / f, y / f, z / f};
    }

    template <typename V>
    constexpr Vec3& operator/=(const V& f) {
        *this = *this / f;
        return *this;
    }

    constexpr T Length2() const {
        return x * x + y * y + z * z;
    }

    // Only implemented for T=float
    float Length() const;
    Vec3 Normalized() const;
    float Normalize(); // returns the previous length, which is often useful

    constexpr T& operator[](std::size_t i) {
        return *((&x) + i);
    }

    constexpr const T& operator[](std::size_t i) const {
        return *((&x) + i);
    }

    constexpr void SetZero() {
        x = 0;
        y = 0;
        z = 0;
    }

    // Common aliases: UVW (texel coordinates), RGB (colors), STQ (texture coordinates)
    constexpr T& u() {
        return x;
    }
    constexpr T& v() {
        return y;
    }
    constexpr T& w() {
        return z;
    }

    constexpr T& r() {
        return x;
    }
    constexpr T& g() {
        return y;
    }
    constexpr T& b() {
        return z;
    }

    constexpr T& s() {
        return x;
    }
    constexpr T& t() {
        return y;
    }
    constexpr T& q() {
        return z;
    }

    constexpr const T& u() const {
        return x;
    }
    constexpr const T& v() const {
        return y;
    }
    constexpr const T& w() const {
        return z;
    }

    constexpr const T& r() const {
        return x;
    }
    constexpr const T& g() const {
        return y;
    }
    constexpr const T& b() const {
        return z;
    }

    constexpr const T& s() const {
        return x;
    }
    constexpr const T& t() const {
        return y;
    }
    constexpr const T& q() const {
        return z;
    }

// swizzlers - create a subvector of specific components
// e.g. Vec2 uv() { return Vec2(x,y); }
// _DEFINE_SWIZZLER2 defines a single such function, DEFINE_SWIZZLER2 defines all of them for all
// component names (x<->r) and permutations (xy<->yx)
#define _DEFINE_SWIZZLER2(a, b, name)                                                              \
    constexpr Vec2<T> name() const {                                                               \
        return Vec2<T>(a, b);                                                                      \
    }
#define DEFINE_SWIZZLER2(a, b, a2, b2, a3, b3, a4, b4)                                             \
    _DEFINE_SWIZZLER2(a, b, a##b);                                                                 \
    _DEFINE_SWIZZLER2(a, b, a2##b2);                                                               \
    _DEFINE_SWIZZLER2(a, b, a3##b3);                                                               \
    _DEFINE_SWIZZLER2(a, b, a4##b4);                                                               \
    _DEFINE_SWIZZLER2(b, a, b##a);                                                                 \
    _DEFINE_SWIZZLER2(b, a, b2##a2);                                                               \
    _DEFINE_SWIZZLER2(b, a, b3##a3);                                                               \
    _DEFINE_SWIZZLER2(b, a, b4##a4)

    DEFINE_SWIZZLER2(x, y, r, g, u, v, s, t);
    DEFINE_SWIZZLER2(x, z, r, b, u, w, s, q);
    DEFINE_SWIZZLER2(y, z, g, b, v, w, t, q);
#undef DEFINE_SWIZZLER2
#undef _DEFINE_SWIZZLER2
};

template <typename T, typename V>
constexpr Vec3<T> operator*(const V& f, const Vec3<T>& vec) {
    return Vec3<T>(f * vec.x, f * vec.y, f * vec.z);
}

template <>
inline float Vec3<float>::Length() const {
    return std::sqrt(x * x + y * y + z * z);
}

template <>
inline Vec3<float> Vec3<float>::Normalized() const {
    return *this / Length();
}

template <>
inline float Vec3<float>::Normalize() {
    float length = Length();
    *this /= length;
    return length;
}

using Vec3f = Vec3<float>;

template <typename T>
class Vec4 {
public:
    T x{};
    T y{};
    T z{};
    T w{};

    constexpr Vec4() = default;
    constexpr Vec4(const T& x_, const T& y_, const T& z_, const T& w_)
        : x(x_), y(y_), z(z_), w(w_) {}

    template <typename T2>
    constexpr Vec4<T2> Cast() const {
        return Vec4<T2>(static_cast<T2>(x), static_cast<T2>(y), static_cast<T2>(z),
                        static_cast<T2>(w));
    }

    static constexpr Vec4 AssignToAll(const T& f) {
        return Vec4(f, f, f, f);
    }

    constexpr Vec4<decltype(T{} + T{})> operator+(const Vec4& other) const {
        return {x + other.x, y + other.y, z + other.z, w + other.w};
    }

    constexpr Vec4& operator+=(const Vec4& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    constexpr Vec4<decltype(T{} - T{})> operator-(const Vec4& other) const {
        return {x - other.x, y - other.y, z - other.z, w - other.w};
    }

    constexpr Vec4& operator-=(const Vec4& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    template <typename U = T>
    constexpr Vec4<std::enable_if_t<std::is_signed_v<U>, U>> operator-() const {
        return {-x, -y, -z, -w};
    }

    constexpr Vec4<decltype(T{} * T{})> operator*(const Vec4& other) const {
        return {x * other.x, y * other.y, z * other.z, w * other.w};
    }

    template <typename V>
    constexpr Vec4<decltype(T{} * V{})> operator*(const V& f) const {
        return {x * f, y * f, z * f, w * f};
    }

    template <typename V>
    constexpr Vec4& operator*=(const V& f) {
        *this = *this * f;
        return *this;
    }

    template <typename V>
    constexpr Vec4<decltype(T{} / V{})> operator/(const V& f) const {
        return {x / f, y / f, z / f, w / f};
    }

    template <typename V>
    constexpr Vec4& operator/=(const V& f) {
        *this = *this / f;
        return *this;
    }

    constexpr T Length2() const {
        return x * x + y * y + z * z + w * w;
    }

    constexpr T& operator[](std::size_t i) {
        return *((&x) + i);
    }

    constexpr const T& operator[](std::size_t i) const {
        return *((&x) + i);
    }

    constexpr void SetZero() {
        x = 0;
        y = 0;
        z = 0;
        w = 0;
    }

    // Common alias: RGBA (colors)
    constexpr T& r() {
        return x;
    }
    constexpr T& g() {
        return y;
    }
    constexpr T& b() {
        return z;
    }
    constexpr T& a() {
        return w;
    }

    constexpr const T& r() const {
        return x;
    }
    constexpr const T& g() const {
        return y;
    }
    constexpr const T& b() const {
        return z;
    }
    constexpr const T& a() const {
        return w;
    }

// Swizzlers - Create a subvector of specific components
// e.g. Vec2 uv() { return Vec2(x,y); }

// _DEFINE_SWIZZLER2 defines a single such function
// DEFINE_SWIZZLER2_COMP1 defines one-component functions for all component names (x<->r)
// DEFINE_SWIZZLER2_COMP2 defines two component functions for all component names (x<->r) and
// permutations (xy<->yx)
#define _DEFINE_SWIZZLER2(a, b, name)                                                              \
    constexpr Vec2<T> name() const {                                                               \
        return Vec2<T>(a, b);                                                                      \
    }
#define DEFINE_SWIZZLER2_COMP1(a, a2)                                                              \
    _DEFINE_SWIZZLER2(a, a, a##a);                                                                 \
    _DEFINE_SWIZZLER2(a, a, a2##a2)
#define DEFINE_SWIZZLER2_COMP2(a, b, a2, b2)                                                       \
    _DEFINE_SWIZZLER2(a, b, a##b);                                                                 \
    _DEFINE_SWIZZLER2(a, b, a2##b2);                                                               \
    _DEFINE_SWIZZLER2(b, a, b##a);                                                                 \
    _DEFINE_SWIZZLER2(b, a, b2##a2)

    DEFINE_SWIZZLER2_COMP2(x, y, r, g);
    DEFINE_SWIZZLER2_COMP2(x, z, r, b);
    DEFINE_SWIZZLER2_COMP2(x, w, r, a);
    DEFINE_SWIZZLER2_COMP2(y, z, g, b);
    DEFINE_SWIZZLER2_COMP2(y, w, g, a);
    DEFINE_SWIZZLER2_COMP2(z, w, b, a);
    DEFINE_SWIZZLER2_COMP1(x, r);
    DEFINE_SWIZZLER2_COMP1(y, g);
    DEFINE_SWIZZLER2_COMP1(z, b);
    DEFINE_SWIZZLER2_COMP1(w, a);
#undef DEFINE_SWIZZLER2_COMP1
#undef DEFINE_SWIZZLER2_COMP2
#undef _DEFINE_SWIZZLER2

#define _DEFINE_SWIZZLER3(a, b, c, name)                                                           \
    constexpr Vec3<T> name() const {                                                               \
        return Vec3<T>(a, b, c);                                                                   \
    }
#define DEFINE_SWIZZLER3_COMP1(a, a2)                                                              \
    _DEFINE_SWIZZLER3(a, a, a, a##a##a);                                                           \
    _DEFINE_SWIZZLER3(a, a, a, a2##a2##a2)
#define DEFINE_SWIZZLER3_COMP3(a, b, c, a2, b2, c2)                                                \
    _DEFINE_SWIZZLER3(a, b, c, a##b##c);                                                           \
    _DEFINE_SWIZZLER3(a, c, b, a##c##b);                                                           \
    _DEFINE_SWIZZLER3(b, a, c, b##a##c);                                                           \
    _DEFINE_SWIZZLER3(b, c, a, b##c##a);                                                           \
    _DEFINE_SWIZZLER3(c, a, b, c##a##b);                                                           \
    _DEFINE_SWIZZLER3(c, b, a, c##b##a);                                                           \
    _DEFINE_SWIZZLER3(a, b, c, a2##b2##c2);                                                        \
    _DEFINE_SWIZZLER3(a, c, b, a2##c2##b2);                                                        \
    _DEFINE_SWIZZLER3(b, a, c, b2##a2##c2);                                                        \
    _DEFINE_SWIZZLER3(b, c, a, b2##c2##a2);                                                        \
    _DEFINE_SWIZZLER3(c, a, b, c2##a2##b2);                                                        \
    _DEFINE_SWIZZLER3(c, b, a, c2##b2##a2)

    DEFINE_SWIZZLER3_COMP3(x, y, z, r, g, b);
    DEFINE_SWIZZLER3_COMP3(x, y, w, r, g, a);
    DEFINE_SWIZZLER3_COMP3(x, z, w, r, b, a);
    DEFINE_SWIZZLER3_COMP3(y, z, w, g, b, a);
    DEFINE_SWIZZLER3_COMP1(x, r);
    DEFINE_SWIZZLER3_COMP1(y, g);
    DEFINE_SWIZZLER3_COMP1(z, b);
    DEFINE_SWIZZLER3_COMP1(w, a);
#undef DEFINE_SWIZZLER3_COMP1
#undef DEFINE_SWIZZLER3_COMP3
#undef _DEFINE_SWIZZLER3
};

template <typename T, typename V>
constexpr Vec4<decltype(V{} * T{})> operator*(const V& f, const Vec4<T>& vec) {
    return {f * vec.x, f * vec.y, f * vec.z, f * vec.w};
}

using Vec4f = Vec4<float>;

template <typename T>
constexpr decltype(T{} * T{} + T{} * T{}) Dot(const Vec2<T>& a, const Vec2<T>& b) {
    return a.x * b.x + a.y * b.y;
}

template <typename T>
constexpr decltype(T{} * T{} + T{} * T{}) Dot(const Vec3<T>& a, const Vec3<T>& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <typename T>
constexpr decltype(T{} * T{} + T{} * T{}) Dot(const Vec4<T>& a, const Vec4<T>& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

template <typename T>
constexpr Vec3<decltype(T{} * T{} - T{} * T{})> Cross(const Vec3<T>& a, const Vec3<T>& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// linear interpolation via float: 0.0=begin, 1.0=end
template <typename X>
constexpr decltype(X{} * float{} + X{} * float{}) Lerp(const X& begin, const X& end,
                                                       const float t) {
    return begin * (1.f - t) + end * t;
}

// linear interpolation via int: 0=begin, base=end
template <typename X, int base>
constexpr decltype((X{} * int{} + X{} * int{}) / base) LerpInt(const X& begin, const X& end,
                                                               const int t) {
    return (begin * (base - t) + end * t) / base;
}

// bilinear interpolation. s is for interpolating x00-x01 and x10-x11, and t is for the second
// interpolation.
template <typename X>
constexpr auto BilinearInterp(const X& x00, const X& x01, const X& x10, const X& x11, const float s,
                              const float t) {
    auto y0 = Lerp(x00, x01, s);
    auto y1 = Lerp(x10, x11, s);
    return Lerp(y0, y1, t);
}

// Utility vector factories
template <typename T>
constexpr Vec2<T> MakeVec(const T& x, const T& y) {
    return Vec2<T>{x, y};
}

template <typename T>
constexpr Vec3<T> MakeVec(const T& x, const T& y, const T& z) {
    return Vec3<T>{x, y, z};
}

template <typename T>
constexpr Vec4<T> MakeVec(const T& x, const T& y, const Vec2<T>& zw) {
    return MakeVec(x, y, zw[0], zw[1]);
}

template <typename T>
constexpr Vec3<T> MakeVec(const Vec2<T>& xy, const T& z) {
    return MakeVec(xy[0], xy[1], z);
}

template <typename T>
constexpr Vec3<T> MakeVec(const T& x, const Vec2<T>& yz) {
    return MakeVec(x, yz[0], yz[1]);
}

template <typename T>
constexpr Vec4<T> MakeVec(const T& x, const T& y, const T& z, const T& w) {
    return Vec4<T>{x, y, z, w};
}

template <typename T>
constexpr Vec4<T> MakeVec(const Vec2<T>& xy, const T& z, const T& w) {
    return MakeVec(xy[0], xy[1], z, w);
}

template <typename T>
constexpr Vec4<T> MakeVec(const T& x, const Vec2<T>& yz, const T& w) {
    return MakeVec(x, yz[0], yz[1], w);
}

// NOTE: This has priority over "Vec2<Vec2<T>> MakeVec(const Vec2<T>& x, const Vec2<T>& y)".
//       Even if someone wanted to use an odd object like Vec2<Vec2<T>>, the compiler would error
//       out soon enough due to misuse of the returned structure.
template <typename T>
constexpr Vec4<T> MakeVec(const Vec2<T>& xy, const Vec2<T>& zw) {
    return MakeVec(xy[0], xy[1], zw[0], zw[1]);
}

template <typename T>
constexpr Vec4<T> MakeVec(const Vec3<T>& xyz, const T& w) {
    return MakeVec(xyz[0], xyz[1], xyz[2], w);
}

template <typename T>
constexpr Vec4<T> MakeVec(const T& x, const Vec3<T>& yzw) {
    return MakeVec(x, yzw[0], yzw[1], yzw[2]);
}

} // namespace Common
