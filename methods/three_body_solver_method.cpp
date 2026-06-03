#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "three_body_solver.hpp"
#include "tasks_queue.hpp"

namespace mm {

template<typename T, typename Wrapper>
int ThreeBodySolverMethodHelper(const nlohmann::json& input,
                                nlohmann::json* output,
                                TasksQueue* tasksQueue);

int ThreeBodySolverMethod(const nlohmann::json& input,
                          nlohmann::json* output,
                          TasksQueue* tasksQueue) {
  std::string valueType = input.at("value_type");
  if (valueType == "float") {
    return ThreeBodySolverMethodHelper<float, FloatAbstractSolverWrapper>(
        input, output, tasksQueue);
  } else if (valueType == "double") {
    return ThreeBodySolverMethodHelper<double, DoubleAbstractSolverWrapper>(
        input, output, tasksQueue);
  }
  return -1;
}

template<typename T, typename Wrapper>
int ThreeBodySolverMethodHelper(const nlohmann::json& input,
                                nlohmann::json* output,
                                TasksQueue* tasksQueue) {
  T tau = input.at("tau");
  T finishTime = input.at("finish_time");
  T exportPeriod = input.at("export_period");

  std::array<T, 3> masses;
  for (size_t i = 0; i < 3; ++i) masses[i] = input.at("masses")[i];

  std::array<T, 3> charges;
  for (size_t i = 0; i < 3; ++i) charges[i] = input.at("charges")[i];

  T c = input.at("c");

  std::vector<std::array<T, 3>> initialPositions(3), initialVelocities(3);
  for (int i = 0; i < 3; ++i) {
    auto& pos = input.at("initial_positions")[i];
    auto& vel = input.at("initial_velocities")[i];
    for (int d = 0; d < 3; ++d) {
      initialPositions[i][d] = pos[d];
      initialVelocities[i][d] = vel[d];
    }
  }

  auto* solver = new ThreeBodySolver<T>(tau, finishTime, exportPeriod,
                                        masses, charges, c,
                                        initialPositions, initialVelocities);
  Wrapper* wrapper = new Wrapper(solver);
  int taskId = tasksQueue->AddTask(wrapper);
  (*output)["id"] = taskId;
  (*output)["status"] = "ok";
  return 0;
}

}  // namespace mm
