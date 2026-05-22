/**
 * @file methods/heat_conduction_solver_method.cpp
 * @author Savva Galkin
 *
 * Парсер входных данных и инициализатор для класса
 * mm::HeatConductionSolver.
 */


#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <heat_conduction_solver.hpp>
#include "tasks_queue.hpp"

using mm::HeatConductionSolver;

namespace mm {

template<typename T, typename Wrapper>
int HeatConductionSolverMethodHelper(
    const nlohmann::json& input,
    nlohmann::json* output,
    TasksQueue* tasksQueue);

int HeatConductionSolverMethod(const nlohmann::json& input,
    nlohmann::json* output, TasksQueue* tasksQueue) {
  std::string valueType = input.at("value_type");

  if (valueType == "float") {
    return HeatConductionSolverMethodHelper
        <float, FloatAbstractSolverWrapper>(input, output, tasksQueue);
  } else if (valueType == "double") {
    return HeatConductionSolverMethodHelper<
        double, DoubleAbstractSolverWrapper>(input, output, tasksQueue);
  }

  return -1;
}

/**
 * @brief Вспомогательная шаблонная функция для парсера входных данных и
 * инициализатора класса mm::HeatConductionSolver.
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
int HeatConductionSolverMethodHelper(
    const nlohmann::json& input,
    nlohmann::json* output,
    TasksQueue* tasksQueue) {
  HeatConductionSolver<T>* solver;

  T tau = input.at("tau");
  T finishTime = input.at("finish_time");
  T exportPeriod = input.at("export_period");
  int M = input.at("M");
  bool rand = input.at("rand");

  if (M < 0 || tau < 0 || finishTime < 0)
    return -1;

  solver = new HeatConductionSolver<T>(tau, finishTime,
      exportPeriod, M, rand);

  Wrapper* wrapper = new Wrapper(solver);

  int taskId = tasksQueue->AddTask(wrapper);

  (*output)["id"] = taskId;
  (*output)["status"] = "ok";

  return 0;
}

}  // namespace mm
