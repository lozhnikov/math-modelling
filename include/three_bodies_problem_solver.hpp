/**
* @file include/three_bodies_problem_solver.hpp
* @author Denis Kirilenko
* 
* Решалка задачи трех тел
*/

#ifndef INCLUDE_THREE_BODIES_PROBLEM_SOLVER_HPP_
#define INCLUDE_THREE_BODIES_PROBLEM_SOLVER_HPP_

#include "abstract_solver.hpp"
#include "matpoint.hpp"

namespace mm {

  /**
   * @brief Решалка задачи трёх тел методом Рунге-Кутты 4-го порядка.
   * @tparam T Тип данных (float/double).
   */
template<typename T>
class ThreeBodyProblemSolver : public AbstractSolver<T> {
 private:
  mat_point<T> bodies[3];  //!< Три материальные точки

  /**
   * @brief Копирует текущее состояние в массив.
   * @param dest Массив из трёх точек для заполнения.
   */
  void GetState(mat_point<T>(&dest)[3]) const;

    /**
     * @brief Устанавливает состояние из массива.
     * @param src Массив с новым состоянием.
     */
  void SetState(const mat_point<T>(&src)[3]);

    /**
     * @brief Вычисляет производные (dx/dt = v, dv/dt = a) для заданного состояния.
     * @param state Текущее состояние (положения, скорости, массы).
     * @param[out] dstate Производные: dstate[i].point = скорость, dstate[i].velocity = ускорение.
     */
  void ComputeDerivatives(const mat_point<T>(&state)[3],
    mat_point<T>(&dstate)[3]) const;

 public:
  /**
   * @brief Конструктор.
   * @param body0 Первое тело.
   * @param body1 Второе тело.
   * @param body2 Третье тело.
   * @param tau Шаг по времени.
   * @param finishTime Конечное время.
   * @param exportPeriod Период экспорта.
   */
  ThreeBodyProblemSolver(const mat_point<T>& body0,
    const mat_point<T>& body1, const mat_point<T>& body2,
    T tau, T finishTime, T exportPeriod);

    bool MakeStep() override;
    void ExportData(nlohmann::json* output) override;
};

}  // namespace mm

#include "three_bodies_problem_solver_impl.hpp"

#endif  // INCLUDE_THREE_BODIES_PROBLEM_SOLVER_HPP_
