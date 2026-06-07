/**
* @file include/matpoint.hpp
* @author Denis Kirilenko
* 
* Класс материальной точки в трехмерном пространстве
*/
#ifndef INCLUDE_MATPOINT_HPP_
#define INCLUDE_MATPOINT_HPP_

#include <iostream>
#include <cmath>
#include "vec3d.hpp"

namespace mm {
// Гравитационная постоянная
const double G = 6.6740e-11;
/**
 * @brief Класс материальной точки
 *
 * @tparam T Тип данных, используемый для арифметики.
 */
template<typename T>

class mat_point {
 public:
  //! Масса
  T mass;
  //! Текущая координата точки
  R3Vec<T> point;
  //! Текущая скорость точки
  R3Vec<T> velocity;
  //! Текущее ускорение точки
  R3Vec<T> acceleration;

  T weight() const {
    return mass;
  }

  // Конструкторы
  mat_point() : point(), velocity(), mass(0), acceleration() {}

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

  // Сила притяжения между двумя точками
  R3Vec<T> NewtonForce(const mat_point& other) const {
    R3Vec<T> res = other.point - this->point;
    T dist = 1 / (res.length() * res.length() * res.length());
    return res * this->mass * other.mass * G * dist;
  }
  // Операторы
  mat_point& operator+= (const mat_point& other) {
    point += other.point;
    velocity += other.velocity;
    acceleration += other.acceleration;
    return *this;
  }

  mat_point operator+ (const mat_point& other) const {
    return mat_point(mass, point + other.point, velocity + other.velocity,
      acceleration + other.acceleration);
  }

  mat_point operator* (const T a) const {
    return mat_point(mass, point * a, velocity * a, acceleration * a);
  }

  // Копирование с произвольным типом данных
  template<typename U>
  mat_point(const mat_point<U>& other) : mass(static_cast<T>(other.mass)),
    point(other.point),
    velocity(other.velocity),
    acceleration(other.acceleration) {
  }
};
}  // namespace mm

#endif  // INCLUDE_MATPOINT_HPP_