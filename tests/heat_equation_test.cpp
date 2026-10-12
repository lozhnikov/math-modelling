/** @file tests/heat_equation_test.cpp */
#include <cmath>
#include <random>
#include "heat_equation.hpp"
#include "test_core.hpp"
#include "test.hpp"

void TestHeatEquation() {
  TestSuite suite("HeatEquation");
  RUN_TEST(suite, []() {
    mm::HeatEquation<double> solver(4, 0.01);
    REQUIRE_EQUAL(solver.GridSize(), 12);
    REQUIRE_CLOSE(solver.Value(12, 4), 4.0, 1e-12);
    REQUIRE_CLOSE(solver.Value(4, 6), -1.0, 1e-12);
    REQUIRE_CLOSE(solver.Value(2, 4), -0.5, 1e-12);
    REQUIRE_CLOSE(solver.Value(0, 10), 7.5, 1e-12);
    REQUIRE(!solver.Inside(2, 6));
  });
  RUN_TEST(suite, []() {
    mm::HeatEquation<double> solver(5, 0.005);
    for (int step = 0; step < 20; ++step) solver.MakeStep();
    for (int i = 0; i <= solver.GridSize(); ++i) {
      for (int j = 0; j <= solver.GridSize(); ++j) {
        REQUIRE(std::isfinite(solver.Value(i, j)));
      }
    }
  });
  RUN_TEST(suite, []() {
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> distribution(2, 12);
    for (int trial = 0; trial < 10; ++trial) {
      int m = distribution(rng);
      double h = 1.0 / m;
      mm::HeatEquation<double> solver(m, h * h / 5.0);
      solver.MakeStep();
      REQUIRE_CLOSE(solver.Value(3 * m, m), 4.0, 1e-12);
      REQUIRE_CLOSE(solver.Value(m, m + 1), -1.0, 1e-12);
    }
  });
  RUN_TEST(suite, []() {
    REQUIRE_THROW(mm::HeatEquation<double>(4, 0.02),
                  std::invalid_argument);
  });
}
