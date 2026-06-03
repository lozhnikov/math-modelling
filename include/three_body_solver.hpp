#ifndef INCLUDE_THREE_BODY_SOLVER_HPP_
#define INCLUDE_THREE_BODY_SOLVER_HPP_

#include <array>
#include <vector>
#include <mutex>
#include <future>
#include <nlohmann/json.hpp>
#include "abstract_solver.hpp"

namespace mm {

template<typename T>
class ThreeBodySolver : public AbstractSolver<T> {
 public:
  ThreeBodySolver(T tau, T finishTime, T exportPeriod,
                  const std::array<T, 3>& masses,
                  const std::array<T, 3>& charges,
                  T c,
                  const std::vector<std::array<T, 3>>& initialPositions,
                  const std::vector<std::array<T, 3>>& initialVelocities);

  bool MakeStep() override;
  void ExportData(nlohmann::json* output) override;

 private:
  std::array<T, 3> masses_;
  std::array<T, 3> charges_;
  T c_;

  std::vector<std::array<T, 3>> positions_;
  std::vector<std::array<T, 3>> velocities_;

  std::vector<std::array<T, 3>> k1_pos_, k1_vel_;
  std::vector<std::array<T, 3>> k2_pos_, k2_vel_;
  std::vector<std::array<T, 3>> k3_pos_, k3_vel_;
  std::vector<std::array<T, 3>> k4_pos_, k4_vel_;

  std::mutex mtx_;

  std::vector<nlohmann::json> frames_;
  T nextExportTime_;

  std::vector<std::array<T, 3>> ComputeAccelerations(
      const std::vector<std::array<T, 3>>& pos) const;

  void Synchronize();
};

}  // namespace mm

#include "three_body_solver_impl.hpp"

#endif  // INCLUDE_THREE_BODY_SOLVER_HPP_
