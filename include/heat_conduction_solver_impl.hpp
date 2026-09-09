/**
 * @file include/heat_conduction_solver_impl.hpp
 * @author Darina Garaeva
 *
 * Решатель уравнения теплопроводности в L-образной области.
 */

#ifndef INCLUDE_HEAT_CONDUCTION_SOLVER_IMPL_HPP_
#define INCLUDE_HEAT_CONDUCTION_SOLVER_IMPL_HPP_

#include <algorithm>
#include <thread>
#include <heat_conduction_solver.hpp>

namespace mm {

template<typename T>
HeatConductionSolver<T>::HeatConductionSolver(
    T tau, T finishTime, T exportPeriod, int M) :
  AbstractSolver<T>(tau, finishTime, exportPeriod),
  M(M),
  h(T(1) / M),
  u((2 * M + 1) * (2 * M + 1)),
  uNext((2 * M + 1) * (2 * M + 1)) {
}

template<typename T>
bool HeatConductionSolver<T>::MakeStep() {
  // Пересчет двумерного индекса в одномерный.
  auto ind = [&](int p, int q) {
    return p * (2 * M + 1) + q;
  };

  auto calculateRows = [&](int first, int last) {
    for (int i = first; i < last; ++i) {
      for (int j = 1; j < 2 * M; ++j) {
        if (i < M && j > M) {
          continue;
        }
        uNext[ind(i, j)] = u[ind(i, j)] + this->tau / h / h *
            (u[ind(i + 1, j)] - 2 * u[ind(i, j)] + u[ind(i - 1, j)] +
             u[ind(i, j + 1)] - 2 * u[ind(i, j)] +
             u[ind(i, j - 1)]);
      }
    }
  };

  const int firstRow = 1;
  const int lastRow = 2 * M;
  const unsigned int hardwareThreads = std::thread::hardware_concurrency();
  const unsigned int maxThreads = M < 50 ? 1 : 4;
  const int threadCount = std::max(1u, std::min(
      std::min(hardwareThreads, maxThreads),
      static_cast<unsigned int>(lastRow - firstRow)));
  std::vector<std::thread> threads;
  threads.reserve(threadCount);
  if (threadCount == 1) {
    calculateRows(firstRow, lastRow);
  } else {
    for (int thread = 0; thread < threadCount; ++thread) {
      int begin = firstRow + thread * (lastRow - firstRow) / threadCount;
      int end = firstRow + (thread + 1) * (lastRow - firstRow) /
          threadCount;
      threads.emplace_back(calculateRows, begin, end);
    }
    for (auto& worker : threads) {
      worker.join();
    }
  }

  for (int j = 0; j <= M; ++j) {
    uNext[ind(0, j)] = -j * h;
  }
  for (int i = M; i <= 2 * M; ++i) {
    uNext[ind(i, 2 * M)] = i * h;
  }
  for (int j = 0; j <= 2 * M; ++j) {
    uNext[ind(2 * M, j)] = 2;
  }
  for (int i = 1; i < 2 * M; ++i) {
    uNext[ind(i, 0)] = uNext[ind(i, 1)];
  }
  for (int i = 1; i < M; ++i) {
    uNext[ind(i, M)] = uNext[ind(i, M - 1)];
  }
  for (int j = M + 1; j < 2 * M; ++j) {
    uNext[ind(M, j)] = uNext[ind(M + 1, j)];
  }

  u = uNext;

  return true;
}

template<typename T>
void HeatConductionSolver<T>::ExportData(
    nlohmann::json* output) {
  (*output)["M"] = M;
  (*output)["fn"];

  for (int i = 0; i <= 2 * M; ++i) {
    for (int j = 0; j <= 2 * M; ++j) {
      (*output)["fn"][i * (2 * M + 1) + j] = u[i * (2 * M + 1) + j];
    }
  }
}


}  // namespace mm

#endif  // INCLUDE_HEAT_CONDUCTION_SOLVER_IMPL_HPP_
