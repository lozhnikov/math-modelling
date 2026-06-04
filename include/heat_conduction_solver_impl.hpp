/**
 * @file include/heat_conduction_solver_impl.hpp
 */

#ifndef INCLUDE_HEAT_CONDUCTION_SOLVER_IMPL_HPP_
#define INCLUDE_HEAT_CONDUCTION_SOLVER_IMPL_HPP_

#include <future>
#include <thread>
//#include <iostream>
#include <heat_conduction_solver.hpp>

namespace mm {

template<typename T>
HeatConductionSolver<T>::
HeatConductionSolver(
    T tau,
    T finishTime,
    T exportPeriod,
    int M)
:
  AbstractSolver<T>(
      tau,
      finishTime,
      exportPeriod),
  M(M),
  h(T(1) / M),
  u((3 * M + 1) * (3 * M + 1), 0),
  uNext((3 * M + 1) * (3 * M + 1), 0) {
}

template<typename T>
bool HeatConductionSolver<T>::IsInside(int i, int j) const {

  T x = j * h;
  T y = i * h;

  if (x > 0.0 &&
      x < 1.0 &&
      y > 2.0 &&
      y < 3.0) {
    return false;
  }

  return true;
}

template<typename T>
void HeatConductionSolver<T>::ApplyBoundaryConditions() {

  int N = 3 * M;

  auto ind = [&](int i, int j) {
        return j * (N + 1) + i;
    };

  // y = 0
  for (int i = 0; i <= N; i++) {
    uNext[ind(i, 0)] = T(0);
  }

  // x = 3
  for (int j = 0; j <= N; j++) {
    uNext[ind(N, j)] = static_cast<T>(j * h);
  }

  // x = 0, y от 0 до 2
  for (int j = 0; j <= 2 * M; j++) {
    uNext[ind(0, j)] = -static_cast<T>(j * h);
  }

  //x = 1, y от 2 до 3
  for (int j = 2 * M; j <= N; j++) {
    uNext[ind(M, j)] = T(1);
  }
  
  //левый верхний угол
  for (int i = 1; i < M; i++) {
    uNext[ind(i, 2 * M)] = uNext[ind(i, 2 * M - 1)];
  }
	
  for (int i = M + 1; i < N; i++) {
    uNext[ind(i, N)] = uNext[ind(i, N - 1)];
  }
}

template<typename T>
bool HeatConductionSolver<T>::MakeStep() {

  int N = 3 * M;

  auto ind = [&](int i, int j) {
        return j * (N + 1) + i;
    };

  uNext = u;

  unsigned threads =
      std::thread::hardware_concurrency();

  if (threads == 0)
    threads = 4;

  std::vector<std::future<void>> futures;

  auto worker =
      [&](int rowBegin,
          int rowEnd) {

    for (int i = rowBegin;
         i < rowEnd;
         i++) {

      for (int j = 1;
           j < N;
           j++) {

        if (!IsInside(j, i))
          continue;

        bool boundary = false;

// внешняя граница
if (i == 0)
  boundary = true;

if (i == N)
  boundary = true;

if (j == 0)
  boundary = true;

if (j == N)
  boundary = true;

if (j >= 2 * M && i == M)
  boundary = true;

if (j == 2 * M && i <= M)
  boundary = true;

if (boundary)
  continue;
        //std::cout << "yo";
        uNext[ind(i, j)] =
            u[ind(i, j)]
            +
            this->tau / (h * h)
            *
            (
              u[ind(i + 1, j)]
              -
              2 * u[ind(i, j)]
              +
              u[ind(i - 1, j)]
            )
            +
            this->tau / (h * h)
            *
            (
              u[ind(i, j + 1)]
              -
              2 * u[ind(i, j)]
              +
              u[ind(i, j - 1)]
            );
      }
    }
  };

  int rowsPerThread = N / threads;

  for (unsigned t = 0; t < threads; t++) {

    int begin = 1 + t * rowsPerThread;

    int end = (t == threads - 1) ? N : begin + rowsPerThread;

    futures.push_back(std::async(std::launch::async, worker, begin, end));
  }

  for (auto& f : futures)
    f.get();

  ApplyBoundaryConditions();

  u.swap(uNext);

  return true;
}

template<typename T>
void HeatConductionSolver<T>::ExportData(nlohmann::json* output) {

  (*output)["M"] = M;

  (*output)["fn"];

  int N = 3 * M;
  (*output)["fn"] = nlohmann::json::array();

  for (int i = 0; i <= N; i++) {
    for (int j = 0; j <= N; j++) {
		
      (*output)["fn"][i * (N + 1) + j] = u[i * (N + 1) + j];
    }
  }
}

}  // namespace mm

#endif
