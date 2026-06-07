/**
* @file methods/three_bodies_problem_method.cpp
* @author Denis Kirilenko
*/

#include <string>
#include <nlohmann/json.hpp>
#include "abstract_solver_wrapper.hpp"
#include "methods.hpp"
#include "three_bodies_problem_solver.hpp"

namespace mm {

int ThreeBodyProblemMethod(const nlohmann::json& input,
  nlohmann::json* output) {
  // Чтение общих параметров
  double tau = input.at("tau").get<double>();
  double finishTime = input.at("finishTime").get<double>();
  double exportPeriod = input.at("exportPeriod").get<double>();
  std::string precision = input.at("precision").get<std::string>();

  // Вспомогательная функция для чтения тела из JSON
  auto readBody = [&](const std::string& key) -> mat_point<double> {
    const auto& obj = input.at(key);
    double mass = obj.at("mass").get<double>();
    const auto& pos = obj.at("position");
    const auto& vel = obj.at("velocity");

    R3Vec<double> position(pos.at("x").get<double>(),
    pos.at("y").get<double>(),
    pos.at("z").get<double>());
    R3Vec<double> velocity(vel.at("x").get<double>(),
    vel.at("y").get<double>(),
    vel.at("z").get<double>());
    R3Vec<double> acceleration(0, 0, 0);

    return mat_point<double>(mass, position, velocity, acceleration);
  };

  mat_point<double> body0 = readBody("body0");
  mat_point<double> body1 = readBody("body1");
  mat_point<double> body2 = readBody("body2");

  AbstractSolverWrapper* wrapper = nullptr;

  if (precision == "float") {
    auto* solver = new ThreeBodyProblemSolver<float>(mat_point<float>(body0),
      mat_point<float>(body1),
      mat_point<float>(body2),
      static_cast<float>(tau),
      static_cast<float>(finishTime),
      static_cast<float>(exportPeriod));

    wrapper = new FloatAbstractSolverWrapper(solver);
  } else if (precision == "double") {
    auto* solver = new ThreeBodyProblemSolver<double>(body0,
    body1, body2, tau, finishTime, exportPeriod);
    wrapper = new DoubleAbstractSolverWrapper(solver);
  } else {
    return -1;  // Неизвестная точность
  }

  bool success = wrapper->Solve(output);
  delete wrapper;
  return success ? 0 : -1;
}

}  // namespace mm