#ifndef INCLUDE_THREE_BODY_SOLVER_IMPL_HPP_
#define INCLUDE_THREE_BODY_SOLVER_IMPL_HPP_

#include <cmath>
#include <future>
#include <vector>
#include "three_body_solver.hpp"

namespace mm {

template<typename T>
ThreeBodySolver<T>::ThreeBodySolver(
    T tau, T finishTime, T exportPeriod,
    const std::array<T, 3>& masses,
    const std::array<T, 3>& charges,
    T c,
    const std::vector<std::array<T, 3>>& initialPositions,
    const std::vector<std::array<T, 3>>& initialVelocities)
    : AbstractSolver<T>(tau, finishTime, exportPeriod),
      masses_(masses), charges_(charges), c_(c),
      positions_(initialPositions), velocities_(initialVelocities),
      k1_pos_(3), k1_vel_(3), k2_pos_(3), k2_vel_(3),
      k3_pos_(3), k3_vel_(3), k4_pos_(3), k4_vel_(3),
      nextExportTime_(exportPeriod) {
  frames_.reserve(static_cast<size_t>(finishTime / exportPeriod) + 1);
}

template<typename T>
std::vector<std::array<T, 3>>
ThreeBodySolver<T>::ComputeAccelerations(
    const std::vector<std::array<T, 3>>& pos) const {
  std::vector<std::array<T, 3>> acc(3, {0, 0, 0});
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      if (i == j) continue;
      T dx = pos[j][0] - pos[i][0];
      T dy = pos[j][1] - pos[i][1];
      T dz = pos[j][2] - pos[i][2];
      T r2 = dx*dx + dy*dy + dz*dz;
      T r = std::sqrt(r2);
      T force = c_ * charges_[i] * charges_[j] / r2;
      acc[i][0] += force / masses_[i] * (dx / r);
      acc[i][1] += force / masses_[i] * (dy / r);
      acc[i][2] += force / masses_[i] * (dz / r);
    }
  }
  return acc;
}

template<typename T>
void ThreeBodySolver<T>::Synchronize() {
  std::lock_guard<std::mutex> lock(mtx_);
}

template<typename T>
bool ThreeBodySolver<T>::MakeStep() {
  T tau = this->tau;

  // ----- k1 -----
  Synchronize();
  {
    std::vector<std::future<void>> futures;
    for (int p = 0; p < 3; ++p) {
      futures.push_back(std::async(std::launch::async,
          [this, p] {
            k1_pos_[p] = velocities_[p];
            auto acc = ComputeAccelerations(positions_);
            k1_vel_[p] = acc[p];
          }));
    }
    for (auto& f : futures) f.get();
  }

  // ----- k2 -----
  std::vector<std::array<T, 3>> pos_mid(3), vel_mid(3);
  for (int p = 0; p < 3; ++p) {
    for (int d = 0; d < 3; ++d) {
      pos_mid[p][d] = positions_[p][d] + (tau/2) * k1_pos_[p][d];
      vel_mid[p][d] = velocities_[p][d] + (tau/2) * k1_vel_[p][d];
    }
  }
  Synchronize();
  {
    std::vector<std::future<void>> futures;
    for (int p = 0; p < 3; ++p) {
      futures.push_back(std::async(std::launch::async,
          [this, p, &pos_mid, &vel_mid] {
            k2_pos_[p] = vel_mid[p];
            auto acc = ComputeAccelerations(pos_mid);
            k2_vel_[p] = acc[p];
          }));
    }
    for (auto& f : futures) f.get();
  }

  // ----- k3 -----
  for (int p = 0; p < 3; ++p) {
    for (int d = 0; d < 3; ++d) {
      pos_mid[p][d] = positions_[p][d] + (tau/2) * k2_pos_[p][d];
      vel_mid[p][d] = velocities_[p][d] + (tau/2) * k2_vel_[p][d];
    }
  }
  Synchronize();
  {
    std::vector<std::future<void>> futures;
    for (int p = 0; p < 3; ++p) {
      futures.push_back(std::async(std::launch::async,
          [this, p, &pos_mid, &vel_mid] {
            k3_pos_[p] = vel_mid[p];
            auto acc = ComputeAccelerations(pos_mid);
            k3_vel_[p] = acc[p];
          }));
    }
    for (auto& f : futures) f.get();
  }

  // ----- k4 -----
  std::vector<std::array<T, 3>> pos_full(3), vel_full(3);
  for (int p = 0; p < 3; ++p) {
    for (int d = 0; d < 3; ++d) {
      pos_full[p][d] = positions_[p][d] + tau * k3_pos_[p][d];
      vel_full[p][d] = velocities_[p][d] + tau * k3_vel_[p][d];
    }
  }
  Synchronize();
  {
    std::vector<std::future<void>> futures;
    for (int p = 0; p < 3; ++p) {
      futures.push_back(std::async(std::launch::async,
          [this, p, &pos_full, &vel_full] {
            k4_pos_[p] = vel_full[p];
            auto acc = ComputeAccelerations(pos_full);
            k4_vel_[p] = acc[p];
          }));
    }
    for (auto& f : futures) f.get();
  }

  // ----- финальное обновление -----
  for (int p = 0; p < 3; ++p) {
    for (int d = 0; d < 3; ++d) {
      positions_[p][d] += (tau/6) * (k1_pos_[p][d] + 2*k2_pos_[p][d] +
                                     2*k3_pos_[p][d] + k4_pos_[p][d]);
      velocities_[p][d] += (tau/6) * (k1_vel_[p][d] + 2*k2_vel_[p][d] +
                                     2*k3_vel_[p][d] + k4_vel_[p][d]);
    }
  }

  // ----- экспорт кадра -----
  T next_time = this->CurrentTime() + tau;
  if (next_time >= nextExportTime_) {
    nlohmann::json frame;
    frame["time"] = next_time;
    frame["positions"] = nlohmann::json::array();
    for (const auto& p : positions_) {
      frame["positions"].push_back({p[0], p[1], p[2]});
    }
    frames_.push_back(frame);
    nextExportTime_ += this->exportPeriod;
  }

  return true;
}

template<typename T>
void ThreeBodySolver<T>::ExportData(nlohmann::json* output) {
  (*output)["data"] = frames_;
}

}  // namespace mm

#endif  // INCLUDE_THREE_BODY_SOLVER_IMPL_HPP_
