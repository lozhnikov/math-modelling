/**
 * @file include/heat_conduction_solver.hpp
 */

#ifndef INCLUDE_HEAT_CONDUCTION_SOLVER_HPP_
#define INCLUDE_HEAT_CONDUCTION_SOLVER_HPP_

#include <vector>
#include <future>
#include <thread>
#include <nlohmann/json.hpp>
#include <abstract_solver.hpp>

namespace mm {

template<typename T>
class HeatConductionSolver :
    public AbstractSolver<T> {
 public:
  HeatConductionSolver(
      T tau,
      T finishTime,
      T exportPeriod,
      int M);

  bool MakeStep() override;
      

  void ExportData(
      nlohmann::json* output) override;

 private:
  int M;

  T h;

  std::vector<T> u;

  std::vector<T> uNext;

  bool IsInside(
      int i,
      int j) const;

  void ApplyBoundaryConditions();
};

}  // namespace mm

#include <heat_conduction_solver_impl.hpp>

#endif