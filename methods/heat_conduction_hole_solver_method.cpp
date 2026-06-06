/**
 * @file methods/heat_conduction_hole_solver_method.cpp
 * @author Mikhail Lozhnikov
 *
 * Парсер входных данных и инициализатор для класса
 * mm::HeatConductionHoleSolver.
 */


#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <heat_conduction_hole_solver.hpp>
#include "tasks_queue.hpp"

using mm::HeatConductionHoleSolver;

namespace mm {

template<typename T, typename Wrapper>
int HeatConductionHoleSolverMethodHelper(
    const nlohmann::json& input,
    nlohmann::json* output,
    TasksQueue* tasksQueue);

int HeatConductionHoleSolverMethod(const nlohmann::json& input,
    nlohmann::json* output, TasksQueue* tasksQueue) {
  std::string valueType = input.at("value_type");

  if (valueType == "float") {
    return HeatConductionHoleSolverMethodHelper
        <float, FloatAbstractSolverWrapper>(input, output, tasksQueue);
  } else if (valueType == "double") {
    return HeatConductionHoleSolverMethodHelper<
        double, DoubleAbstractSolverWrapper>(input, output, tasksQueue);
  }

  return -1;
}

/**
 * @brief Вспомогательная шаблонная функция для парсера входных данных и
 * инициализатора класса mm::HeatConductionHoleSolver.
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
int HeatConductionHoleSolverMethodHelper(
    const nlohmann::json& input,
    nlohmann::json* output,
    TasksQueue* tasksQueue) {
  HeatConductionHoleSolver<T>* solver;

  T tau = input.at("tau");
  T finishTime = input.at("finish_time");
  T exportPeriod = input.at("export_period");
  int M = input.at("M");

  if (M <= 0 || tau < 0 || finishTime < 0)
    return -1;

  T h = T(1) / M;

  if (tau > h * h / 4)
    return -1;

  solver = new HeatConductionHoleSolver<T>(tau, finishTime,
      exportPeriod, M);

  Wrapper* wrapper = new Wrapper(solver);

  int taskId = tasksQueue->AddTask(wrapper);

  (*output)["id"] = taskId;
  (*output)["status"] = "ok";

  return 0;
}

}  // namespace mm
