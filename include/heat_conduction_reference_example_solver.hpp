/**
 * @file include/heat_conduction_reference_example_solver.hpp
 * @author Mikhail Lozhnikov
 *
 * Эталонный пример реализации решалки для уравнения теплопроводности.
 */

#ifndef INCLUDE_HEAT_CONDUCTION_REFERENCE_EXAMPLE_SOLVER_HPP_
#define INCLUDE_HEAT_CONDUCTION_REFERENCE_EXAMPLE_SOLVER_HPP_

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
class HeatConductionReferenceExampleSolver : public AbstractSolver<T> {
 public:
  /**
   * @brief Конструктор калсса.
   * @param tau Величина шага по времени.
   * @param finishTime Конечное время.
   * @param exportPeriod Временной интервал для сохранения данных.
   * @param M Число отрезков разбиения по каждой координате.
   */
  HeatConductionReferenceExampleSolver(T tau, T finishTime, T exportPeriod,
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

#include <heat_conduction_reference_example_solver_impl.hpp>

#endif  // INCLUDE_HEAT_CONDUCTION_REFERENCE_EXAMPLE_SOLVER_HPP_



