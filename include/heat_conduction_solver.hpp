/**
 * @file include/heat_conduction_solver.hpp
 * @author Savva Galkin
 *
 * Реализация решалки для уравнения теплопроводности.
 */

#ifndef INCLUDE_HEAT_CONDUCTION_SOLVER_HPP_
#define INCLUDE_HEAT_CONDUCTION_SOLVER_HPP_

#include <vector>
#include <thread>
#include <random>
#include <nlohmann/json.hpp>
#include <abstract_solver.hpp>

namespace mm {

/**
 * @brief Абстрактный класс решалки.
 *
 * @tparam T Тип данных, используемый для арифметики.
 */
template<typename T>
class HeatConductionSolver : public AbstractSolver<T> {
 public:
  /**
   * @brief Конструктор класса.
   * @param tau Величина шага по времени.
   * @param finishTime Конечное время.
   * @param exportPeriod Временной интервал для сохранения данных.
   * @param M Число отрезков разбиения по каждой координате.
   * @param rand Случайность/неслучайность начальных данных.
   */
  HeatConductionSolver(T tau, T finishTime, T exportPeriod,
      int M, bool rand);

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

#include <heat_conduction_solver_impl.hpp>

#endif  // INCLUDE_HEAT_CONDUCTION_SOLVER_HPP_