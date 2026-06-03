#ifndef INCLUDE_MATPOINT_HPP_
#define INCLUDE_MATPOINT_HPP_

#include <iostream>
#include <cmath>
#include "vec3d.hpp"
G = 6.6740e-11;

template<typename T>

class mat_point {
public:
  T mass;
  R3Vec<T> point;
  R3Vec<T> velocity;
  R3Vec<T> acceleration;

  T weight() const {
	  return mass;
  }
  
  mat_point() : point(), velocity(), mass(0), acceleration() {};
  mat_point(const mat_point& other) {
    point = other.point;
    velocity = other.velocity;
    mass = other.mass;
	acceleration = other.acceleration;
  }
  mat_point(T m, R3Vec<T> a, R3Vec<T> b, R3Vec<T> c) {
    mass = m;
	point = a;
	velocity = b;
	acceleration = c;
  }
  
  R3Vec<T> NewtonForce(const mat_point& other) const {
	R3Vec<T> res = other.point - this->point;
	T dist = pow(res.length(), -3);
	return res * this->mass * other.mass * G * dist;
  }
  
  mat_point& operator+= (const mat_point& other) {
	point += other.point;
	velocity += other.velocity;
	acceleration += other.acceleration;
	return *this;
  }

  mat_point operator+ (const mat_point& other) const {
	return mat_point(mass, point + other.point, velocity + other.velocity, acceleration + other.acceleration);
  }

  mat_point operator* (const T a) const {
	return mat_point(mass, point * a, velocity * a, acceleration * a);
  }
};

#endif //! INCLUDE_MATPOINT_HPP_