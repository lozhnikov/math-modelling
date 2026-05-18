/**
 * @file include/heat_conduction_l_reverse_solver_impl.hpp
 * @author Zhuravlev George
 *
 *  Решалка для уравнения теплопроводности в L-образной области.
 */

#ifndef INCLUDE_HEAT_CONDUCTION_L_REVERSE_SOLVER_IMPL_HPP_
#define INCLUDE_HEAT_CONDUCTION_L_REVERSE_SOLVER_IMPL_HPP_

#include <thread>
#include <vector>
#include <heat_conduction_l_reverse_solver.hpp>

namespace mm {

template<typename T>
HeatConductionLReverseSolver<T>::HeatConductionLReverseSolver(
    T tau, T finishTime, T exportPeriod, int M, int numThreads) :
  AbstractSolver<T>(tau, finishTime, exportPeriod),
  M(M),
  h(T(1) / M),
  numThreads(numThreads),
  u((2 * M + 1) * (2 * M + 1)),
  uNext((2 * M + 1) * (2 * M + 1)) {
}

template<typename T>
void HeatConductionLReverseSolver<T>::ComputeInteriorStrip(
    int startRow, int endRow) {
  // Пересчет двумерного индекса в одномерный
  auto ind = [this](int p, int q) {
    return p * (2 * M + 1) + q;
  };

  for (int i = startRow; i < endRow; i++) {
    for (int j = 1; j < 2 * M; j++) {
      // Узлы с i <= M и j <= M лежат либо в вырезе, либо на внутреннем
      // уступе и пересчитываются как граничные.
      if (i <= M && j <= M)
        continue;

      // Внутренние точки L-образной области
      uNext[ind(i, j)] =
        u[ind(i, j)] + this->tau / h / h * (
            u[ind(i, j + 1)] - 2 * u[ind(i, j)] + u[ind(i, j - 1)]) +
          this->tau / h / h * (
            u[ind(i + 1, j)] - 2 * u[ind(i, j)] + u[ind(i - 1, j)]);
    }
  }
}

template<typename T>
bool HeatConductionLReverseSolver<T>::MakeStep() {
  // Пересчет двумерного индекса в одномерный
  auto ind = [this](int p, int q) {
    return p * (2 * M + 1) + q;
  };

  // Разбиение строк внутреннего цикла между потоками.
  std::vector<std::thread> threads;
  threads.reserve(numThreads);

  int numInteriorRows = 2 * M - 1;

  for (int t = 0; t < numThreads; t++) {
    int startRow = 1 + t * numInteriorRows / numThreads;
    int endRow = 1 + (t + 1) * numInteriorRows / numThreads;

    threads.emplace_back(
        &HeatConductionLReverseSolver<T>::ComputeInteriorStrip,
        this, startRow, endRow);
  }

  for (auto& thread : threads) {
    thread.join();
  }

  // Верхняя граница y = 2 вместе с углами (Дирихле u = 2)
  for (int j = 0; j <= 2 * M; j++) {
    uNext[ind(2 * M, j)] = 2;
  }

  // Левая часть левой границы x = 0, y в [1, 2)
  // (Дирихле u = 4 - y, без верхнего угла)
  for (int i = M; i < 2 * M; i++) {
    uNext[ind(i, 0)] = 4 - i * h;
  }

  // Правая часть нижней границы y = 0, x в [1, 2]
  // (Дирихле u = 4 * x, с правым нижним углом)
  for (int j = M; j <= 2 * M; j++) {
    uNext[ind(0, j)] = 4 * j * h;
  }

  // Внутренняя вертикальная часть уступа x = 1, y в (0, 1]
  // (Дирихле u = 2 * y, с внутренним углом, без нижнего)
  for (int i = 1; i <= M; i++) {
    uNext[ind(i, M)] = 2 * i * h;
  }

  // Правая граница x = 2 (Нейман, без верхнего и нижнего углов)
  for (int i = 1; i < 2 * M; i++) {
    uNext[ind(i, 2 * M)] = uNext[ind(i, 2 * M - 1)];
  }

  // Внутренняя горизонтальная часть уступа y = 1, x в (0, 1)
  // (Нейман, без левого и внутреннего углов)
  for (int j = 1; j < M; j++) {
    uNext[ind(M, j)] = uNext[ind(M + 1, j)];
  }

  u = uNext;

  return true;
}

template<typename T>
void HeatConductionLReverseSolver<T>::ExportData(
    nlohmann::json* output) {
  (*output)["M"] = M;
  (*output)["fn"];

  for (int i = 0; i <= 2 * M; i++) {
    for (int j = 0; j <= 2 * M; j++) {
      if (i < M && j < M) {
        // Узел внутри вырезанного левого нижнего подквадрата.
        (*output)["fn"][i * (2 * M + 1) + j] = nullptr;
      } else {
        (*output)["fn"][i * (2 * M + 1) + j] = u[i * (2 * M + 1) + j];
      }
    }
  }
}


}  // namespace mm

#endif  // INCLUDE_HEAT_CONDUCTION_L_REVERSE_SOLVER_IMPL_HPP_
