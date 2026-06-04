#ifndef INCLUDE_IMPLICIT_HEAT_CONDUCTION_SOLVER_HPP_
#define INCLUDE_IMPLICIT_HEAT_CONDUCTION_SOLVER_HPP_

#include <vector>
#include <nlohmann/json.hpp>
#include <abstract_solver.hpp>

namespace mm {

template<typename T>
class ImplicitHeatConductionSolver : public AbstractSolver<T> {
 public:
  ImplicitHeatConductionSolver(T tau, T finishTime, T exportPeriod, int M);
  bool MakeStep() override;
  void ExportData(nlohmann::json* output) override;

 private:
  int M;
  T h;
  std::vector<T> u;
  std::vector<T> uNext;
};

}  // namespace mm

#include <implicit_heat_conduction_solver_impl.hpp>

#endif  // INCLUDE_IMPLICIT_HEAT_CONDUCTION_SOLVER_HPP_
