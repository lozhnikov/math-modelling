/**
 * @file include/heat_conduction_l_reverse_solver.hpp
 * @author Zhuravlev George
 *
 * Решалка для уравнения теплопроводности в L-образной области.
 */

#ifndef INCLUDE_HEAT_CONDUCTION_L_REVERSE_SOLVER_HPP_
#define INCLUDE_HEAT_CONDUCTION_L_REVERSE_SOLVER_HPP_

#include <vector>
#include <thread>
#include <nlohmann/json.hpp>
#include <abstract_solver.hpp>

namespace mm {

/**
 * @brief Решалка для уравнения теплопроводности в L-образной области.
 *
 * @tparam T Тип данных, используемый для арифметики.
 */
template<typename T>
class HeatConductionLReverseSolver : public AbstractSolver<T> {
 public:
  /**
   * @brief Конструктор класса.
   * @param tau Величина шага по времени.
   * @param finishTime Конечное время.
   * @param exportPeriod Временной интервал для сохранения данных.
   * @param M Число отрезков разбиения на единицу длины.
   * @param numThreads Число потоков для расчёта.
   */
  HeatConductionLReverseSolver(T tau, T finishTime, T exportPeriod,
      int M, int numThreads);

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
  /**
   * @brief Пересчёт схемы для полосы строк сетки.
   * @param startRow Первая строка полосы (включительно).
   * @param endRow Конечная строка полосы (не включительно).
   *
   * Функция вызывается из отдельного потока в MakeStep().
   */
  void ComputeInteriorStrip(int startRow, int endRow);

  //! Число отрезков разбиения на единицу длины.
  int M;
  //! Шаг сетки.
  T h;
  //! Число потоков для расчёта.
  int numThreads;
  //! Решение на текущем шаге по времени.
  std::vector<T> u;
  //! Решение на следующем шаге по времени.
  std::vector<T> uNext;
};

}  // namespace mm

#include <heat_conduction_l_reverse_solver_impl.hpp>

#endif  // INCLUDE_HEAT_CONDUCTION_L_REVERSE_SOLVER_HPP_
