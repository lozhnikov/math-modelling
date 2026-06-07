/**
* @file include/three_bodies_problem_solver_impl.hpp
* @author Denis Kirilenko
*/

#ifndef INCLUDE_THREE_BODIES_PROBLEM_SOLVER_IMPL_HPP_
#define INCLUDE_THREE_BODIES_PROBLEM_SOLVER_IMPL_HPP_

#include "three_bodies_problem_solver.hpp"

namespace mm {
template<typename T>
void ThreeBodyProblemSolver<T>::GetState(mat_point<T>(&dest)[3]) const {
  for (int i = 0; i < 3; ++i)
    dest[i] = bodies[i];
}

template<typename T>
void ThreeBodyProblemSolver<T>::SetState(const mat_point<T>(&src)[3]) {
  for (int i = 0; i < 3; ++i)
    bodies[i] = src[i];
}

template<typename T>
void ThreeBodyProblemSolver<T>::ComputeDerivatives
  (const mat_point<T>(&state)[3],
  mat_point<T>(&dstate)[3]) const {
  for (int i = 0; i < 3; i++) {
    dstate[i].point = state[i].velocity;
    dstate[i].velocity = R3Vec<T>(0, 0, 0);
    dstate[i].mass = state[i].mass;
  }
  for (int i = 0; i < 3; i++)
    for (int j = i + 1; j < 3; j++) {
      R3Vec<T> force = state[i].NewtonForce(state[j]);
      dstate[i].velocity += force * (1.0 / state[i].mass);
      dstate[j].velocity += force * (-1.0 / state[j].mass);
    }
}

template<typename T>
ThreeBodyProblemSolver<T>::ThreeBodyProblemSolver(const mat_point<T>& body0,
  const mat_point<T>& body1, const mat_point<T>& body2,
  T tau, T finishTime, T exportPeriod): AbstractSolver<T>
    (tau, finishTime, exportPeriod) {
  bodies[0] = body0;
  bodies[1] = body1;
  bodies[2] = body2;
}

template<typename T>
bool ThreeBodyProblemSolver<T>::MakeStep() {
  mat_point<T> state[3];
  GetState(state);
  mat_point<T> k1[3], k2[3], k3[3], k4[3];
  mat_point<T> temp[3];

  ComputeDerivatives(state, k1);

  for (int i = 0; i < 3; i++) {
    temp[i] = state[i];
    temp[i].point = state[i].point + k1[i].point * (this->tau / 2.0);
    temp[i].velocity = state[i].velocity + k1[i].velocity * (this->tau / 2.0);
  }
  ComputeDerivatives(temp, k2);

  for (int i = 0; i < 3; i++) {
    temp[i] = state[i];
    temp[i].point = state[i].point + k2[i].point * (this->tau / 2.0);
    temp[i].velocity = state[i].velocity + k2[i].velocity * (this->tau / 2.0);
  }
  ComputeDerivatives(temp, k3);
  for (int i = 0; i < 3; i++) {
    temp[i] = state[i];
    temp[i].point = state[i].point + k3[i].point * this->tau;
    temp[i].velocity = state[i].velocity + k3[i].velocity * this->tau;
  }
  ComputeDerivatives(temp, k4);

  mat_point<T> new_state[3];
  for (int i = 0; i < 3; ++i) {
    new_state[i] = state[i];
    new_state[i].point = state[i].point + (k1[i].point +
    k2[i].point * 2.0 + k3[i].point * 2.0 + k4[i].point) * (this->tau / 6.0);
    new_state[i].velocity = state[i].velocity +
    (k1[i].velocity + k2[i].velocity * 2.0
    + k3[i].velocity * 2.0 + k4[i].velocity) * (this->tau / 6.0);
  }

  SetState(new_state);
  return true;
}

template<typename T>
void ThreeBodyProblemSolver<T>::ExportData(nlohmann::json* output) {
  nlohmann::json& out = *output;
  for (int i = 0; i < 3; i++) {
    out["bodies"][i]["mass"] = bodies[i].mass;
    out["bodies"][i]["position"]["x"] = bodies[i].point.X();
    out["bodies"][i]["position"]["y"] = bodies[i].point.Y();
    out["bodies"][i]["position"]["z"] = bodies[i].point.Z();
    out["bodies"][i]["velocity"]["x"] = bodies[i].velocity.X();
    out["bodies"][i]["velocity"]["y"] = bodies[i].velocity.Y();
    out["bodies"][i]["velocity"]["z"] = bodies[i].velocity.Z();
  }
}
}  // namespace mm

#endif  // INCLUDE_THREE_BODIES_PROBLEM_SOLVER_IMPL_HPP_
