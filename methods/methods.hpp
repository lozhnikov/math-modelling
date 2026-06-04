/**
 * @file methods/methods.hpp
 * @author BorisKlimov
 *
 * Объявления серверных методов (парсеров) для алгоритмов.
 */

#ifndef METHODS_METHODS_HPP_
#define METHODS_METHODS_HPP_

#include <nlohmann/json.hpp>
#include "tasks_queue.hpp"

namespace mm {

int ImplicitHeatConductionSolverMethod(
  const nlohmann::json& input,
  nlohmann::json* output,
  TasksQueue* tasksQueue);

}  // namespace mm

#endif  // METHODS_METHODS_HPP_
