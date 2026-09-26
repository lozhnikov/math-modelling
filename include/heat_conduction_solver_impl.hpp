/**
 * @file abstract_solver_impl.hpp
 * @author Mikhail Kashcheev
 */

#ifndef INCLUDE_HEAT_CONDUCTION_SOLVER_IMPL_HPP_
#define INCLUDE_HEAT_CONDUCTION_SOLVER_IMPL_HPP_

#include <thread>

#include <heat_conduction_solver.hpp>

namespace mm {

template<typename T>
HeatConductionSolver<T>::
    HeatConductionSolver(T tau, T finishTime, T exportPeriod, int M):

  AbstractSolver<T>(tau, finishTime, exportPeriod),
  M(M),
  h(T(1) / M),
  u((3 * M + 1) * (3 * M + 1), 0),
  uNext((3 * M + 1) * (3 * M + 1), 0){
  }


template<typename T>
bool HeatConductionSolver<T>::
    IsInside(int i, int j) const{
  T x = j * h;
  T y = i * h;
  if (x > 1.0 && x < 2.0 && y > 1.0 && y < 2.0 || x > 3.0 || x < 0.0 || y > 3.0 || y < 0.0 ){
    return false;
  }

  return true;
}


template<typename T>
void HeatConductionSolver<T>::ApplyBoundaryConditions(){
  int N = 3 * M;
  auto ind = [&](int i, int j) {
    return i * (N + 1) + j;
  };


  //вертикальные границы, слева направо
  for (int i = 0; i <= N; i++){
    T y = i * h;
    uNext[ind(i, 0)] = T(2) - y;
  }
  for (int i = M; i <= 2 * M; i++){
    T y = i * h;
    uNext[ind(i, M)] = T(-1) + y;
  }
  for (int i = M; i <= 2 * M; i++){
    //T y = i * h;
    uNext[ind(i, 2 * M)] = uNext[ind(i, 2 * M + 1)];
  }
  for (int i = 0; i <= N; i++){
    uNext[ind(i, N)] = T(2);
  }


  //горизонтальные границы, снизу вверх
  for (int j = 0; j <= N; j++){
    uNext[ind(0, j)] = uNext[ind(1, j)];
  }
  for (int j = M; j <= 2 * M; j++){
    uNext[ind(M, j)] = uNext[ind(M - 1, j)];
  }
  for (int j = M; j <= 2 * M; j++){
    uNext[ind(2 * M, j)] = T(1);
  }
  for (int j = 0; j <= N; j++){
    T x = j * h;
    uNext[ind(N, j)] = T(-1) + x;
  }

    }

template<typename T>
bool HeatConductionSolver<T>::
MakeStep() {

  int N = 3 * M;

  auto ind =
      [&](int i, int j) {
        return i * (N + 1) + j;
      };

  uNext = u;

  unsigned threads_number =
      std::thread::hardware_concurrency();

  if (threads_number == 0)
    threads_number = 4;

  unsigned maxThreads =
      static_cast<unsigned>(N - 1);

  if (threads_number > maxThreads)
    threads_number = maxThreads;

  std::vector<std::thread> threads;

  auto worker =
      [&](int rowBegin,
          int rowEnd) {

    for (int i = rowBegin; i < rowEnd; i++) {
      for (int j = 1; j < N; j++) {

        if (!IsInside(i, j))
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

// левая стенка выреза x = 1
if (j == M && i >= M && i <= 2 * M)
  boundary = true;

// правая стенка выреза x = 2
if (j == 2 * M && i >= M && i <= 2 * M)
  boundary = true;

// верхняя стенка выреза y = 2
if (i == 2 * M && j >= M && j <= 2 * M)
  boundary = true;

// нижняя стенка выреза y = 2
if (i == M && j >= M && j <= 2 * M)
  boundary = true;

if (boundary)
  continue;

  T coef = this->tau / (h * h);

  uNext[ind(i, j)] = u[ind(i, j)] +
  coef * (u[ind(i + 1, j)] - 2 * u[ind(i, j)] + u[ind(i - 1, j)]) +
  coef * (u[ind(i, j + 1)] - 2 * u[ind(i, j)] + u[ind(i, j - 1)]);
      }
    }
  };



  int rows = N - 1;

  int rowsPerThread =
      rows / threads_number;

  int remainder =
      rows % threads_number;

  int currentRow = 1;

  for (unsigned t = 0; t < threads_number; t++) {

    int count =
        rowsPerThread +
        (t < static_cast<unsigned>(remainder) ? 1 : 0);

    int begin1 =
        currentRow;

    int end2 =
        begin1 + count;

    currentRow = end2;

    threads.emplace_back(
      worker,
      begin1,
      end2);
  }



  for (auto& thread : threads){
    thread.join();
  }

  ApplyBoundaryConditions();
  u.swap(uNext);

  return true;
}

template<typename T>
void HeatConductionSolver<T>::
ExportData(
  nlohmann::json* output) {
  (*output)["M"] = M;
  (*output)["fn"];

  int N = 3 * M;

  (*output)["fn"] =
      nlohmann::json::array();

  for (int i = 0; i <= N; i++) {
    for (int j = 0; j <= N; j++) {

      (*output)["fn"][i * (N + 1) + j] = u[i * (N + 1) + j];
    }
  }
}

}  // namespace mm

#endif  // INCLUDE_HEAT_CONDUCTION_SOLVER_IMPL_HPP_



