/** @file methods/heat_equation_method.cpp */
#include <cmath>
#include <exception>
#include <nlohmann/json.hpp>
#include "heat_equation.hpp"
#include "methods.hpp"

namespace mm {
int HeatEquationMethod(const nlohmann::json& input, nlohmann::json* output) {
  try {
    const int m = input.at("m").get<int>();
    const double finish = input.at("finish_time").get<double>();
    const double initial = input.value("initial_temperature", 0.0);
    const int exportEvery = input.value("export_every", 10);
    if (m < 2 || m > 100 || !std::isfinite(finish) || finish < 0 ||
        !std::isfinite(initial) || exportEvery < 1) {
      throw std::invalid_argument("Invalid input parameters");
    }
    const double h = 1.0 / m;
    const double tau = input.value("tau", h * h / 5.0);
    HeatEquation<double> solver(m, tau, initial);
    const int steps = static_cast<int>(std::ceil(finish / tau));
    if (steps > 100000) {
      throw std::invalid_argument("Too many time steps");
    }
    (*output)["h"] = h;
    (*output)["tau"] = tau;
    (*output)["m"] = m;
    (*output)["data"] = nlohmann::json::array();
    auto save = [&](int step) {
      (*output)["data"].push_back({{"time", step * tau},
                                      {"data", solver.Snapshot()}});
    };
    save(0);
    for (int step = 1; step <= steps; ++step) {
      solver.MakeStep();
      if (step % exportEvery == 0 || step == steps) save(step);
    }
    (*output)["status"] = "ok";
    return 0;
  } catch (const std::exception& error) {
    *output = {{"status", "failed"}, {"error", error.what()}};
    return 1;
  }
}
}  // namespace mm
