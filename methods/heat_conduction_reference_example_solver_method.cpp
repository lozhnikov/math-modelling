/**
 * @file methods/heat_conduction_reference_example_solver_method.cpp
 * @author Mikhail Lozhnikov
 *
 * Парсер входных данных и инициализатор для класса
 * mm::HeatConductionReferenceExampleSolver.
 */


#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <heat_conduction_reference_example_solver.hpp>
#include "tasks_queue.hpp"

using mm::HeatConductionReferenceExampleSolver;

namespace mm {

template<typename T, typename Wrapper>
int HeatConductionReferenceExampleSolverMethodHelper(
    const nlohmann::json& input,
    nlohmann::json* output,
    TasksQueue* tasksQueue);

int HeatConductionReferenceExampleSolverMethod(const nlohmann::json& input,
    nlohmann::json* output, TasksQueue* tasksQueue) {
  std::string valueType = input.at("value_type");

  if (valueType == "float") {
    return HeatConductionReferenceExampleSolverMethodHelper
        <float, FloatAbstractSolverWrapper>(input, output, tasksQueue);
  } else if (valueType == "double") {
    return HeatConductionReferenceExampleSolverMethodHelper<
        double, DoubleAbstractSolverWrapper>(input, output, tasksQueue);
  }

  return -1;
}

/**
 * @brief Вспомогательная шаблонная функция для парсера входных данных и
 * инициализатора класса mm::HeatConductionReferenceExampleSolver.
 *
 * @tparam T Тип данных для вещественной арифметики.
 * @param input Входные данные в формате JSON.
 * @param output Выходные данные в формате JSON.
 * @param tasksQueue Очередь задач.
 * @return Функция возвращает 0 в случае успеха и отрицательное число
 * если входные данные заданы некорректно.
 *
 * Функция запускает алгоритм алгоритм расчета уравнения теплопроводности,
 * используя входные данные в JSON формате. Результат также выдаётся
 * в JSON формате.
 */
template<typename T, typename Wrapper>
int HeatConductionReferenceExampleSolverMethodHelper(
    const nlohmann::json& input,
    nlohmann::json* output,
    TasksQueue* tasksQueue) {
  HeatConductionReferenceExampleSolver<T>* solver;

  T tau = input.at("tau");
  T finishTime = input.at("finish_time");
  T exportPeriod = input.at("export_period");
  int M = input.at("M");

  if (M < 0 || tau < 0 || finishTime < 0)
    return -1;

  solver = new HeatConductionReferenceExampleSolver<T>(tau, finishTime,
      exportPeriod, M);

  Wrapper* wrapper = new Wrapper(solver);

  int taskId = tasksQueue->AddTask(wrapper);

  (*output)["id"] = taskId;
  (*output)["status"] = "ok";

  return 0;
}

}  // namespace mm
