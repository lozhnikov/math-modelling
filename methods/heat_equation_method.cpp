/**
 * @file methods/heat_equation_method.cpp
 *
 * Серверный метод для решалки уравнения теплопроводности.
 */

#include <exception>
#include <fstream>
#include <nlohmann/json.hpp>
#include <abstract_solver_wrapper.hpp>
#include <heat_equation.hpp>
#include "methods.hpp"

namespace mm {

int HeatEquationMethod(const nlohmann::json& input,
                       nlohmann::json* output) {
  try {
    const int n = input.value("n", 30);
    const double finishTime = input.value("finish_time", 0.2);
    const double exportPeriod = input.value("export_period", 0.01);
    const int numThreads = input.value("num_threads", 4);

    HeatEquationSolver<double> solver(
      n,
      finishTime,
      exportPeriod,
      numThreads);

    if (!solver.Solve(output)) {
      (*output)["status"] = "failed";
      return -1;
    }

    (*output)["status"] = "ok";

    if (input.contains("output_file")) {
      std::ofstream file(input["output_file"].get<std::string>());
      file << output->dump(2);
    }

    return 0;
  } catch (const std::exception& exception) {
    (*output)["status"] = "failed";
    (*output)["error"] = exception.what();
    return -1;
  }
}

}  // namespace mm
