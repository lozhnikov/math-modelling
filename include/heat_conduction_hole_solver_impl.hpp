/**
 * @file include/heat_conduction_hole_solver_impl.hpp
 * @author Mikhail Lozhnikov
 *
 *  Эталонный пример реализации решалки для уравнения теплопроводности.
 */

#ifndef INCLUDE_HEAT_CONDUCTION_HOLE_SOLVER_IMPL_HPP_
#define INCLUDE_HEAT_CONDUCTION_HOLE_SOLVER_IMPL_HPP_

#include <heat_conduction_hole_solver.hpp>

namespace mm {

template<typename T>
HeatConductionHoleSolver<T>::HeatConductionHoleSolver(
    T tau, T finishTime, T exportPeriod, int M) :
  AbstractSolver<T>(tau, finishTime, exportPeriod),
  M(M),
  h(T(1) / M),
  u((3 * M + 1) * (3 * M + 1)),
  uNext((3 * M + 1) * (3 * M + 1)) {
  ApplyBoundaryConditions(&u);
  ApplyBoundaryConditions(&uNext);
}

template<typename T>
int HeatConductionHoleSolver<T>::GridSize() const {
  return 3 * M;
}

template<typename T>
int HeatConductionHoleSolver<T>::Index(int i, int j) const {
  return i * (GridSize() + 1) + j;
}

template<typename T>
bool HeatConductionHoleSolver<T>::IsInsideHole(
    int i, int j) const {
  return i > M && i < 2 * M && j > M && j < 2 * M;
}

template<typename T>
bool HeatConductionHoleSolver<T>::IsHoleBoundary(
    int i, int j) const {
  const bool onHorizontalSide = (i == M || i == 2 * M) &&
      j >= M && j <= 2 * M;
  const bool onVerticalSide = (j == M || j == 2 * M) &&
      i >= M && i <= 2 * M;

  return onHorizontalSide || onVerticalSide;
}

template<typename T>
bool HeatConductionHoleSolver<T>::IsOuterBoundary(
    int i, int j) const {
  return i == 0 || i == GridSize() || j == 0 || j == GridSize();
}

template<typename T>
bool HeatConductionHoleSolver<T>::IsBoundary(int i, int j) const {
  return IsOuterBoundary(i, j) || IsHoleBoundary(i, j);
}

template<typename T>
void HeatConductionHoleSolver<T>::ApplyBoundaryConditions(
    std::vector<T>* values) const {
  const int size = GridSize();

  for (int j = 0; j <= size; j++) {
    (*values)[Index(0, j)] = T(-2);
    (*values)[Index(size, j)] = T(1);
  }

  for (int i = 1; i < size; i++) {
    const T y = i * h;

    (*values)[Index(i, 0)] = (*values)[Index(i, 1)];
    (*values)[Index(i, size)] = T(-2) + y;
  }

  for (int j = M; j <= 2 * M; j++) {
    const T x = j * h;

    (*values)[Index(M, j)] = T(3) - x;
    (*values)[Index(2 * M, j)] = (*values)[Index(2 * M + 1, j)];
  }

  for (int i = M; i <= 2 * M; i++) {
    (*values)[Index(i, M)] = T(2);
    (*values)[Index(i, 2 * M)] = T(1);
  }

  for (int i = M + 1; i < 2 * M; i++) {
    for (int j = M + 1; j < 2 * M; j++) {
      (*values)[Index(i, j)] = T(0);
    }
  }
}

template<typename T>
bool HeatConductionHoleSolver<T>::MakeStep() {
  const int size = GridSize();
  const T coeff = this->tau / h / h;

  for (int i = 1; i < size; i++) {
    for (int j = 1; j < size; j++) {
      if (IsInsideHole(i, j) || IsBoundary(i, j)) {
        continue;
      }

      uNext[Index(i, j)] =
          u[Index(i, j)] + coeff * (
              u[Index(i, j - 1)] - 2 * u[Index(i, j)] +
              u[Index(i, j + 1)]) + coeff * (
              u[Index(i - 1, j)] - 2 * u[Index(i, j)] +
              u[Index(i + 1, j)]);
    }
  }

  ApplyBoundaryConditions(&uNext);
  u = uNext;

  return true;
}

template<typename T>
void HeatConductionHoleSolver<T>::ExportData(
    nlohmann::json* output) {
  (*output)["M"] = M;
  (*output)["grid_size"] = GridSize();
  (*output)["fn"];

  for (int i = 0; i <= GridSize(); i++) {
    for (int j = 0; j <= GridSize(); j++) {
      (*output)["fn"][Index(i, j)] = u[Index(i, j)];
    }
  }
}

}  // namespace mm

#endif  // INCLUDE_HEAT_CONDUCTION_HOLE_SOLVER_IMPL_HPP_
