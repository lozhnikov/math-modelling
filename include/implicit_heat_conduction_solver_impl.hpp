/**
 * @file include/implicit_heat_conduction_solver_impl.hpp
 * @author BorisKlimov
 * @brief Реализация неявного метода решения уравнения теплопроводности.
 */

#ifndef INCLUDE_IMPLICIT_HEAT_CONDUCTION_SOLVER_IMPL_HPP_
#define INCLUDE_IMPLICIT_HEAT_CONDUCTION_SOLVER_IMPL_HPP_

#include <implicit_heat_conduction_solver.hpp>
#include <future>
#include <cmath>
#include <algorithm>
#include <vector>
#include <stdexcept>

namespace mm {

template<typename T>
ImplicitHeatConductionSolver<T>::ImplicitHeatConductionSolver(
    T tau, T finishTime, T exportPeriod, int M) :
  AbstractSolver<T>(tau, finishTime, exportPeriod),
  M(M),
  h(M > 0 ? T(1) / M : T(1)),
  u((M + 1) * (M + 1)),
  uNext((M + 1) * (M + 1)) {
  auto ind = [&](int p, int q) { return p * (M + 1) + q; };

  if (M <= 0) {
    throw std::invalid_argument("M must be positive");
  }

  // Начальное заполнение сетки при t = 0
  for (int i = 0; i <= M; ++i) {
    for (int j = 0; j <= M; ++j) {
      u[ind(i, j)] = T(-1);
    }
  }

  // Граничные условия (начальный момент времени)
  // Верхняя граница (y = 1): u = -x
  for (int j = 0; j <= M; ++j) {
    u[ind(M, j)] = -j * h;
  }

  // Левая граница (x = 0): u = y - 1
  // Правая граница (x = 1): u = -1
  for (int i = 0; i <= M; ++i) {
    u[ind(i, 0)] = i * h - T(1);
    u[ind(i, M)] = T(-1);
  }

  // Нижняя граница (y = 0): du/dy = 0 (условие Неймана)
  for (int j = 1; j < M; ++j) {
    u[ind(0, j)] = u[ind(1, j)];
  }

  uNext = u;
}

template<typename T>
bool ImplicitHeatConductionSolver<T>::MakeStep() {
  auto ind = [&](int p, int q) { return p * (M + 1) + q; };
  T factor = this->tau / (h * h);
  T eps = T(1e-5);
  int maxIters = 500;

  std::vector<T> uOldIter = uNext;

  // Итерационный метод Якоби для неявной разностной схемы
  for (int iter = 0; iter < maxIters; ++iter) {
    std::vector<std::future<void>> futures;

    // Распараллеливание вычислений по строкам
    for (int i = 1; i < M; ++i) {
      futures.push_back(std::async(std::launch::async,
        [i, this, factor, ind, &uOldIter]() {
          for (int j = 1; j < M; ++j) {
            this->uNext[ind(i, j)] = (this->u[ind(i, j)] + factor * (
              uOldIter[ind(i + 1, j)] + uOldIter[ind(i - 1, j)] +
              uOldIter[ind(i, j + 1)] + uOldIter[ind(i, j - 1)])) /
              (T(1) + T(4) * factor);
          }
        }));
    }

    // Ожидание завершения всех потоков
    for (auto& f : futures) {
      f.wait();
    }

    // Применение граничных условий к новому приближению
    // Верхняя граница: u = -x
    for (int j = 0; j <= M; ++j) {
      uNext[ind(M, j)] = -j * h;
    }

    // Левая и правая границы
    for (int i = 1; i <= M; ++i) {
      uNext[ind(i, 0)] = i * h - T(1);  // u = y - 1
      uNext[ind(i, M)] = T(-1);         // u = -1
    }

    // Нижняя граница: du/dy = 0
    for (int j = 1; j < M; ++j) {
      uNext[ind(0, j)] = uNext[ind(1, j)];
    }

    // Проверка сходимости метода Якоби
    T maxDiff = T(0);
    for (int i = 1; i < M; ++i) {
      for (int j = 1; j < M; ++j) {
        maxDiff = std::max(maxDiff,
          std::abs(uNext[ind(i, j)] - uOldIter[ind(i, j)]));
      }
    }

    uOldIter = uNext;

    if (maxDiff < eps) {
      break;
    }
  }

  u = uNext;

  // Обновление текущего времени
  this->t += this->tau;

  return true;
}

template<typename T>
void ImplicitHeatConductionSolver<T>::ExportData(nlohmann::json* output) {
  (*output)["M"] = M;
  (*output)["fn"] = nlohmann::json::array();

  for (int i = 0; i <= M; ++i) {
    for (int j = 0; j <= M; ++j) {
      (*output)["fn"].push_back(u[i * (M + 1) + j]);
    }
  }
}

}  // namespace mm

#endif  // INCLUDE_IMPLICIT_HEAT_CONDUCTION_SOLVER_IMPL_HPP_
