/**
 * @file methods/heat_conduction_l_reverse_solver_method.cpp
 * @author Zhuravlev George
 *
 * Парсер входных данных и инициализатор для класса
 * mm::HeatConductionLReverseSolver.
 */


#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <heat_conduction_l_reverse_solver.hpp>
#include "tasks_queue.hpp"

using mm::HeatConductionLReverseSolver;

namespace mm {

template<typename T, typename Wrapper>
int HeatConductionLReverseSolverMethodHelper(
    const nlohmann::json& input,
    nlohmann::json* output,
    TasksQueue* tasksQueue);

int HeatConductionLReverseSolverMethod(const nlohmann::json& input,
    nlohmann::json* output, TasksQueue* tasksQueue) {
  std::string valueType = input.at("value_type");

  if (valueType == "float") {
    return HeatConductionLReverseSolverMethodHelper
        <float, FloatAbstractSolverWrapper>(input, output, tasksQueue);
  } else if (valueType == "double") {
    return HeatConductionLReverseSolverMethodHelper<
        double, DoubleAbstractSolverWrapper>(input, output, tasksQueue);
  }

  return -1;
}

/**
 * @brief Вспомогательная шаблонная функция для парсера входных данных и
 * инициализатора класса mm::HeatConductionLReverseSolver.
 *
 * @tparam T Тип данных для вещественной арифметики.
 * @param input Входные данные в формате JSON.
 * @param output Выходные данные в формате JSON.
 * @param tasksQueue Очередь задач.
 * @return Функция возвращает 0 в случае успеха и отрицательное число
 * если входные данные заданы некорректно.
 *
 * Функция запускает алгоритм расчета уравнения теплопроводности в L-образной
 * области, используя входные данные в JSON формате. Результат также выдаётся
 * в JSON формате.
 */
template<typename T, typename Wrapper>
int HeatConductionLReverseSolverMethodHelper(
    const nlohmann::json& input,
    nlohmann::json* output,
    TasksQueue* tasksQueue) {
  HeatConductionLReverseSolver<T>* solver;

  T tau = input.at("tau");
  T finishTime = input.at("finish_time");
  T exportPeriod = input.at("export_period");
  int M = input.at("M");
  int numThreads = input.at("num_threads");

  if (M < 0 || tau < 0 || finishTime < 0 || numThreads < 1)
    return -1;

  solver = new HeatConductionLReverseSolver<T>(tau, finishTime,
      exportPeriod, M, numThreads);

  Wrapper* wrapper = new Wrapper(solver);

  int taskId = tasksQueue->AddTask(wrapper);

  (*output)["id"] = taskId;
  (*output)["status"] = "ok";

  return 0;
}

}  // namespace mm
