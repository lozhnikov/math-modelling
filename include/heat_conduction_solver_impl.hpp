/**
 * @file include/heat_conduction_solver_impl.hpp
 * @author Savva Galkin
 *
 *  Реализация решалки для уравнения теплопроводности.
 */

#ifndef INCLUDE_HEAT_CONDUCTION_SOLVER_IMPL_HPP_
#define INCLUDE_HEAT_CONDUCTION_SOLVER_IMPL_HPP_

#include <heat_conduction_solver.hpp>

namespace mm {

template<typename T>
HeatConductionSolver<T>::HeatConductionSolver(
    T tau, T finishTime, T exportPeriod, int M, bool rand) :
  AbstractSolver<T>(tau, finishTime, exportPeriod),
  M(M),
  h(T(1) / M),
  u((M + 1) * (M + 1)),
  uNext((M + 1) * (M + 1)) {
  if (rand) {
    for (int i = 0; i < (M+1)*(M+1); i++) {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_real_distribution<double> dis(0.0, 1.0);
      u[i] = dis(gen) + 1;
    }
  }
}

const int num_threads = 3;

template<typename T>
bool HeatConductionSolver<T>::MakeStep() {
  // Пересчет двумерного индекса в одномерный для всех ячеек решётки
  auto ind = [&](int p, int q) {
    return p * (M + 1) + q;
  };

  auto indInner = [&](int p, int q) {
  // Пересчет двумерного индекса в одномерный для ячеек решётки,
  // на которые наложены линейные условия
    return p * M + q;
  };

  auto indMatrix = [&](int p, int q) {
  // Пересчет двумерного индекса в одномерный
  // для элементов матрицы неявной схемы
    return p * (M - 1)*M + q;
  };

  // Создаём матрицу A и вектор y для последующего решения уравнения Ax = y,
  // всё инициализируем нулями для удобства
  int* A = new int[M*M*(M-1)*(M-1)]{};
  int* y = new int[M*(M-1)]{};

  for (int j = 0; j <= M; j++) {
    // Нижняя граница вместе с нижними углами
    uNext[ind(0, j)] = 1;
  }

  for (int i = 1; i <= M; i++) {
    // Левая граница вместе с верхними углами
    uNext[ind(i, 0)] = 1 + i * h;
  }

  for (int j = 1; j < M; j++) {
    // Верхняя граница
    uNext[ind(M, j)] = 2 - j * h;
  }

  // При неявной разностной схеме приходится решать уравнение вида Ax = y
  // Здесь заполняются квадратная матрица А со стороной М*(М-1) и вектор у,
  // состоящий из М*(М-1) координат

  for (int i = 0; i < M-1; i++) {
    for (int j = 0; j <= M-1; j++) {
      // Внутренние точки области
      if (j == M-1) {
        // Условие du/dx = 0 на правой границе
        A[indMatrix(indInner(i, j),indInner(i, j))] = 1;
        A[indMatrix(indInner(i, j),indInner(i, j-1))] = -1;
      }
      else {
        if (j == 0) {
          // Условие u = 1 + y на левой границе
          y[indInner(i, j)] -= (1 + i * h) / h / h;
        }
        else {
          A[indMatrix(indInner(i, j),indInner(i, j - 1))] = 1 / h / h;
        }

        A[indMatrix(indInner(i, j),indInner(i, j + 1))] = 1 / h / h;

        if (i == 0) {
          // Условие u = 1 на нижней границе
          y[indInner(i, j)] -= 1 / h / h;
        }
        else {
          A[indMatrix(indInner(i, j),indInner(i-1, j))] = 1 / h / h;
        }

        if (i == M - 2) {
          // Условие u = 2 - x на верхней границе
          y[indInner(i, j)] -= (2 - j * h) / h / h;
        }
        else {
          A[indMatrix(indInner(i, j),indInner(i + 1, j))] = 1 / h / h;
        }

        A[indMatrix(indInner(i, j),indInner(i, j))] = (- 4 / h / h);
        A[indMatrix(indInner(i, j),indInner(i, j))] -= (1 / this->tau);
        y[indInner(i, j)] -= u[ind(i + 1, j + 1)]/this->tau;
      }
    }
  }

  // Вычислим вектор x, обратив матрицу A

  auto eliminate_rows = [A, y, indMatrix](int n, int M,
    int pivot_row, int start_row, int end_row) {
    for (int i = start_row; i < end_row; ++i) {
        if (i != pivot_row) {
            double factor = A[indMatrix(i, pivot_row)]
            factor /= A[indMatrix(pivot_row, pivot_row)];
            for (int j = pivot_row; j < n; ++j) {
                A[indMatrix(i, j)] -= factor * A[indMatrix(pivot_row, j)];
            }
            y[i] -= factor*y[pivot_row];
        }
    }
  };

  for (int i = 0; i < M*(M-1); ++i) {
      double divisor = A[indMatrix(i, i)];
      for (int j = i; j < M*(M-1); ++j) A[indMatrix(i, j)] /= divisor;
      y[i] /= divisor;

      std::vector<std::thread> threads;
      int rows_per_thread = M*(M-1) / num_threads;
      for (int t = 0; t < num_threads; ++t) {
          int start = t * rows_per_thread;
          int end = (t == num_threads - 1) ? M*(M-1) : (t + 1) * rows_per_thread;
          threads.emplace_back(eliminate_rows, M*(M-1), M, i, start, end);
      }

      for (auto& th : threads) th.join();
  }

  // Подставляем в ответ

  for (int i = 0; i < M-1; i++) {
    for (int j = 0; j <= M-1; j++) {
      uNext[ind(i+1, j+1)] = y[indInner(i, j)];
    }
  }

  delete[] A;
  delete[] y;

  u = uNext;

  return true;
}

template<typename T>
void HeatConductionSolver<T>::ExportData(
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

#endif  // INCLUDE_HEAT_CONDUCTION_SOLVER_IMPL_HPP_