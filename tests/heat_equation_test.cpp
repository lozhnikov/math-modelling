/**
 * @file tests/heat_equation_test.cpp
 *
 * Тесты для решалки уравнения теплопроводности.
 */

#include <cmath>
#include <fstream>
#include <random>
#include <string>
#include <nlohmann/json.hpp>
#include <heat_equation.hpp>
#include "test.hpp"
#include "test_core.hpp"

namespace {

void TestStabilityCondition() {
  mm::HeatEquationSolver<double> solver(30, 0.1, 0.01, 2);

  REQUIRE(solver.Tau() <= solver.H() * solver.H() / 4.0);
}

void TestDirichletBoundaryValues() {
  mm::HeatEquationSolver<double> solver(30, 0.1, 0.01, 2);
  const int n = solver.GridSize() - 1;

  REQUIRE_CLOSE(solver.Value(0, 0), 2.0, 1e-12);
  REQUIRE_CLOSE(solver.Value(n, 0), 2.0, 1e-12);
  REQUIRE_CLOSE(solver.Value(n, n), -1.0, 1e-12);
  REQUIRE_CLOSE(solver.Value(0, n), -1.0, 1e-12);
  REQUIRE_CLOSE(solver.Value(0, n / 3), 3.0, 1e-12);
  REQUIRE_CLOSE(solver.Value(0, 2 * n / 3), -1.0, 1e-12);
}

void TestNeumannBoundaryValues() {
  mm::HeatEquationSolver<double> solver(30, 0.1, 0.01, 2);
  const int n = solver.GridSize() - 1;

  solver.MakeStep();

  REQUIRE_CLOSE(solver.Value(n / 2, n), solver.Value(n / 2, n - 1), 1e-12);
  REQUIRE_CLOSE(
    solver.Value(n / 3, n / 2),
    solver.Value(n / 3 + 1, n / 2),
    1e-12);
}

void TestExportContainsCutOut() {
  mm::HeatEquationSolver<double> solver(30, 0.1, 0.02, 2);
  nlohmann::json output;

  REQUIRE(solver.Solve(&output));

  const nlohmann::json& frame = output["data"][0]["data"];
  REQUIRE(frame["values"][15][5].is_null());
  REQUIRE(!frame["values"][15][10].is_null());
}

void TestInitialConditionExport() {
  mm::HeatEquationSolver<double> solver(30, 0.1, 0.02, 2);
  nlohmann::json output;

  REQUIRE(solver.Solve(&output));

  const nlohmann::json& frame = output["data"][0]["data"];
  REQUIRE_CLOSE(output["data"][0]["time"].get<double>(), 0.0, 1e-12);
  REQUIRE_CLOSE(frame["values"][15][15].get<double>(), 0.0, 1e-12);
}

void TestRandomSmallGrid() {
  std::mt19937 gen(12);
  std::uniform_int_distribution<int> distribution(2, 6);

  const int n = 3 * distribution(gen);
  mm::HeatEquationSolver<double> solver(n, 0.05, 0.01, 2);
  nlohmann::json output;

  REQUIRE(solver.Solve(&output));
  REQUIRE_EQUAL(output["data"].empty(), false);
}

void TestRemoteHeatEquation(httplib::Client* cli) {
  nlohmann::json input;

  input["n"] = 12;
  input["finish_time"] = 0.02;
  input["export_period"] = 0.01;
  input["num_threads"] = 2;
  input["output_file"] = "data/heat_equation_method.json";

  httplib::Result res = cli->Post(
    "/HeatEquation",
    input.dump(),
    "application/json");

  REQUIRE(res != nullptr);
  REQUIRE_EQUAL(res->status, 200);

  nlohmann::json output = nlohmann::json::parse(res->body);
  REQUIRE_EQUAL(output["status"], "ok");

  std::ofstream file("data/heat_equation.json");
  file << output.dump(2);
}

}  // namespace

void TestHeatEquation(httplib::Client* cli) {
  TestSuite suite("HeatEquation");

  RUN_TEST(suite, TestStabilityCondition);
  RUN_TEST(suite, TestDirichletBoundaryValues);
  RUN_TEST(suite, TestNeumannBoundaryValues);
  RUN_TEST(suite, TestExportContainsCutOut);
  RUN_TEST(suite, TestInitialConditionExport);
  RUN_TEST(suite, TestRandomSmallGrid);
  RUN_TEST_REMOTE(suite, cli, TestRemoteHeatEquation);
}
