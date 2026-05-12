/**
 * @file include/heat_conduction_reference_example_solver_impl.hpp
 * @author Mikhail Lozhnikov
 *
 *  Эталонный пример реализации решалки для уравнения теплопроводности.
 */

#ifndef INCLUDE_HEAT_CONDUCTION_REFERENCE_EXAMPLE_SOLVER_IMPL_HPP_
#define INCLUDE_HEAT_CONDUCTION_REFERENCE_EXAMPLE_SOLVER_IMPL_HPP_

#include <heat_conduction_reference_example_solver.hpp>

namespace mm {

template<typename T>
HeatConductionReferenceExampleSolver<T>::HeatConductionReferenceExampleSolver(
    T tau, T finishTime, T exportPeriod, int M) :
  AbstractSolver<T>(tau, finishTime, exportPeriod),
  M(M),
  h(T(1) / M),
  u((M + 1) * (M + 1)),
  uNext((M + 1) * (M + 1)) {
}

template<typename T>
bool HeatConductionReferenceExampleSolver<T>::MakeStep() {
  // Пересчет двумерного индекса в одномерный
  auto ind = [&](int p, int q) {
    return p * (M + 1) + q;
  };

  // #pragma omp parallel for
  for (int i = 1; i < M; i++) {
    for (int j = 1; j < M; j++) {
      // Внутренние точки области
      uNext[ind(i, j)] =
        u[ind(i, j)] + this->tau / h / h * (
            u[ind(i, j + 1)] - 2 * u[ind(i, j)] + u[ind(i, j - 1)]) +
          this->tau / h / h * (
            u[ind(i + 1, j)] - 2 * u[ind(i, j)] + u[ind(i - 1, j)]);
    }
  }

  for (int j = 0; j <= M; j++) {
    // Нижняя граница вместе с нижними углами
    uNext[ind(0, j)] = 5;
  }

  for (int i = 1; i <= M; i++) {
    // Левая граница вместе с верхними углами
    uNext[ind(i, 0)] = 5 + i * h;

    // Правая граница вместе с верхними углами
    uNext[ind(i, M)] = 5 - i * h;
  }

  for (int j = 1; j < M; j++) {
    // Верхняя граница
    uNext[ind(M, j)] = uNext[ind(M - 1, j)];
  }

  u = uNext;

  return true;
}

template<typename T>
void HeatConductionReferenceExampleSolver<T>::ExportData(
    nlohmann::json* output) {
  (*output)["M"] = M;
  (*output)["fn"];

  for (int i = 0; i <= M; i++) {
    for (int j = 0; j <= M; j++) {
      (*output)["fn"][i * (M + 1) + j] = u[i * (M + 1) + j];
    }
  }
}


}  // namespace mm

#endif  // INCLUDE_HEAT_CONDUCTION_REFERENCE_EXAMPLE_SOLVER_IMPL_HPP_



