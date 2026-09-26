/**
 * @file include/heat_conduction_solver.hpp
 * @author Mikhail Kashcheev
 */

#ifndef INCLUDE_HEAT_CONDUCTION_SOLVER_HPP_
#define INCLUDE_HEAT_CONDUCTION_SOLVER_HPP_

#include <vector>
#include <thread>

#include <nlohmann/json.hpp>
#include <abstract_solver.hpp>


namespace mm {

/**
 * @brief класс решалки.
 *
 * @tparam T Тип данных, используемый для арифметики.
 */
template<typename T>
class HeatConductionSolver : public AbstractSolver<T> {
  public:
    HeatConductionSolver(T tau, T finishTime, T exportPeriod, int M);
    bool MakeStep() override;
    void ApplyBoundaryConditions();
    void ExportData(nlohmann::json* output) override;


  private :
    int M; //масштаб сетки; h = 1/3M
    T h;
    std::vector<T> u;
    std::vector<T> uNext;

  bool IsInside(int i, int j)  const;
  //void ApplyBoundaryConditions();
};

}  // namespace mm

#include <heat_conduction_solver_impl.hpp>

#endif  // INCLUDE_ABSTRACT_SOLVER_HPP_

