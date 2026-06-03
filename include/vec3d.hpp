#ifndef INCLUDE_VEC3D_HPP_
#define INCLUDE_VEC3D_HPP_

#include <iostream>
#include <cmath>

namespace mm {

    template<typename T>
    class R3Vec {
    private:
        T x, y, z;
    public:
        R3Vec() {
            x = 0;
            y = 0;
            z = 0;
        }
        R3Vec(const R3Vec& other) {
            x = other.x;
            y = other.y;
            z = other.z;
        }
        R3Vec(T xarg, T yarg, T zarg) {
            x = xarg;
            y = yarg;
            z = zarg;
        }

        T X() const { return x; }
        T& X() { return x; }
        T Y() const { return y; }
        T& Y() { return y; }
        T Z() const { return z; }
        T& Z() { return z; }

        T length() const {
            return sqrt(x * x + y * y + z * z);
        }

        R3Vec operator + (const R3Vec& other) const {
            return R3Vec(x + other.x, y + other.y, z + other.z);
        }
        R3Vec& operator += (const R3Vec& other) {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }
        R3Vec operator - (const R3Vec& other) const {
            return R3Vec(x - other.x, y - other.y, z - other.z);
        }
        R3Vec operator -() const {
            return R3Vec(-x, -y, -z);
        }
        R3Vec& operator -= (const R3Vec& other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return *this;
        }
        R3Vec operator^ (const R3Vec& other) const {
            return R3Vec(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
        }
        R3Vec operator* (const T a) const {
            return R3Vec(x * a, y * a, z * a);
        }
        double operator& (const R3Vec& other) const {
            return x * other.x + y * other.y + z * other.z;
        }
        bool operator== (const R3Vec& other) const {
            return bool(!((abs(x - other.x) > 1e-10) || (abs(y - other.y) > 1e-10) || (abs(z - other.z) > 1e-10)));
        }
        bool operator!= (const R3Vec& other) const {
            return bool(!(other == *this));
        }
        bool operator> (const R3Vec& other) const {
            return bool(x - other.x > 1e-10) && (y - other.y > 1e-10) && (z - other.z > 1e-10);
        }
        bool operator<= (const R3Vec& other) const {
            return bool(!(*this > other));
        }
        bool operator< (const R3Vec& other) const {
            return bool(x - other.x < 1e-10) && (y - other.y < 1e-10) && (z - other.z < 1e-10);
        }
        bool operator>= (const R3Vec& other) const {
            return bool(!(*this < other));
        }
        R3Vec normalize() const {
            return *this * (1.0 / length());
        }
    };
    std::istream& operator>> (std::istream& in, R3Vec& vec);
    std::ostream& operator<< (std::ostream& out, const R3Vec& vec);
}

#endif //! INCLUDE_VEC3D_HPP_