/**
 * @file abstract_solver_wrapper.hpp
 * @author Mikhail Lozhnikov
 *
 * Здесь определена оболочка для абстрактной решалки над различными типами
 * данных.
 */

#ifndef INCLUDE_ABSTRACT_SOLVER_WRAPPER_HPP_
#define INCLUDE_ABSTRACT_SOLVER_WRAPPER_HPP_


#include <abstract_solver.hpp>

namespace mm {

/**
 * @brief Класс оболочки над абстрактной решалкой.
 */
class AbstractSolverWrapper {
 public:
  /**
   * @brief Функция решает задачу и сохраняет решение в выходной JSON.
   *
   * @param output Выходной JSON.
   * @return Функция возвращает true в случае успешного нахождения решения и
   * false в противном случае.
   */
  virtual bool Solve(nlohmann::json* output) = 0;

  /**
   * @brief Деструктор.
   */
  virtual ~AbstractSolverWrapper() { }
};

/**
 * @brief Класс оболочки над абстрактной решалкой для типа данных float.
 */
class FloatAbstractSolverWrapper : public AbstractSolverWrapper {
 private:
  //! Абстрактная решалка для типа данных float.
  AbstractSolver<float>* solver;

 public:
  /**
   * @brief Конструктор.
   * @param solver Абстрактная решалка для типа данных float.
   */
  explicit FloatAbstractSolverWrapper(AbstractSolver<float>* solver) :
    solver(solver)
  { }

  /**
   * @brief Функция решает задачу и сохраняет решение в выходной JSON.
   *
   * @param output Выходной JSON.
   * @return Функция возвращает true в случае успешного нахождения решения и
   * false в противном случае.
   */
  bool Solve(nlohmann::json* output) override {
    return solver->Solve(output);
  }

  /**
   * @brief Деструктор.
   */
  ~FloatAbstractSolverWrapper() {
    delete solver;
  }
};

/**
 * @brief Класс оболочки над абстрактной решалкой для типа данных double.
 */
class DoubleAbstractSolverWrapper : public AbstractSolverWrapper {
 private:
  //! Абстрактная решалка для типа данных float.
  AbstractSolver<double>* solver;

 public:
  /**
   * @brief Конструктор.
   * @param solver Абстрактная решалка для типа данных double.
   */
  explicit DoubleAbstractSolverWrapper(AbstractSolver<double>* solver) :
    solver(solver)
  { }

  /**
   * @brief Функция решает задачу и сохраняет решение в выходной JSON.
   *
   * @param output Выходной JSON.
   * @return Функция возвращает true в случае успешного нахождения решения и
   * false в противном случае.
   */
  bool Solve(nlohmann::json* output) override {
    return solver->Solve(output);
  }

  /**
   * @brief Деструктор.
   */
  ~DoubleAbstractSolverWrapper() {
    delete solver;
  }
};

}  // namespace mm

#endif  // INCLUDE_ABSTRACT_SOLVER_WRAPPER_HPP_
