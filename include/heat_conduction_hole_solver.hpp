/**
 * @file include/heat_conduction_hole_solver.hpp
 * @author Mikhail Lozhnikov
 *
 * Эталонный пример реализации решалки для уравнения теплопроводности.
 */

#ifndef INCLUDE_HEAT_CONDUCTION_HOLE_SOLVER_HPP_
#define INCLUDE_HEAT_CONDUCTION_HOLE_SOLVER_HPP_

#include <vector>
#include <nlohmann/json.hpp>
#include <abstract_solver.hpp>

namespace mm {

/**
 * @brief Абстрактный класс решалки.
 *
 * @tparam T Тип данных, используемый для арифметики.
 */
template<typename T>
class HeatConductionHoleSolver : public AbstractSolver<T> {
 public:
  /**
   * @brief Конструктор калсса.
   * @param tau Величина шага по времени.
   * @param finishTime Конечное время.
   * @param exportPeriod Временной интервал для сохранения данных.
   * @param M Число отрезков разбиения по каждой координате.
   */
  HeatConductionHoleSolver(T tau, T finishTime, T exportPeriod,
      int M);

  /**
   * @brief Выполнить один шаг по времени.
   *
   * @return Функция возвращает true в случае успешного выполнения шага и
   *  false в противном случае.
   *
   */
  bool MakeStep() override;

  /**
   * @brief Получить данные на текущем шаге по времени.
   *
   * @param output Выходной JSON.
   */
  void ExportData(nlohmann::json* output) override;

 private:
  int GridSize() const;
  int Index(int i, int j) const;
  bool IsInsideHole(int i, int j) const;
  bool IsHoleBoundary(int i, int j) const;
  bool IsOuterBoundary(int i, int j) const;
  bool IsBoundary(int i, int j) const;
  void ApplyBoundaryConditions(std::vector<T>* values) const;

  //! Число отрезков разбиения по каждой координате.
  int M;
  //! Шаг сетки.
  T h;
  //! Решение на текущем шаге по времени.
  std::vector<T> u;
  //! Решение на следующем шаге по времени.
  std::vector<T> uNext;
};

}  // namespace mm

#include <heat_conduction_hole_solver_impl.hpp>

#endif  // INCLUDE_HEAT_CONDUCTION_HOLE_SOLVER_HPP_


