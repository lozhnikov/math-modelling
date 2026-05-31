/**
 * @file include/heat_equation.hpp
 *
 * Явная конечно-разностная решалка для двумерного уравнения
 * теплопроводности.
 */

#ifndef INCLUDE_HEAT_EQUATION_HPP_
#define INCLUDE_HEAT_EQUATION_HPP_

#include <algorithm>
#include <cmath>
#include <future>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>
#include <nlohmann/json.hpp>
#include <abstract_solver.hpp>

namespace mm {

/**
 * @brief Решалка уравнения u_t = u_xx + u_yy в квадрате с вырезом.
 *
 * Расчетная область: [0, 3] x [0, 3] без прямоугольника
 * [0, 1] x [1, 2]. Используется явная схема. Она устойчива, поскольку
 * tau = 0.24 * h^2 <= h^2 / 4.
 *
 * @tparam T Тип данных для арифметики.
 */
template<typename T>
class HeatEquationSolver : public AbstractSolver<T> {
 public:
  using ValueType = T;

  HeatEquationSolver(int n, T finishTime, T exportPeriod, int numThreads) :
    AbstractSolver<T>(
      T(0.24) * T(3.0 / n) * T(3.0 / n),
      finishTime,
      exportPeriod),
    n(n),
    size(n + 1),
    numThreads(std::max(1, numThreads)),
    h(T(3.0) / static_cast<T>(n)),
    values(size * size, T(0)),
    nextValues(size * size, T(0)) {
    if (n < 3 || n % 3 != 0) {
      throw std::runtime_error(
        "n должно делиться на 3 и быть не меньше 3");
    }
    if (finishTime <= T(0)) {
      throw std::runtime_error("finish_time должно быть положительным");
    }
    if (exportPeriod <= T(0)) {
      throw std::runtime_error("export_period должно быть положительным");
    }

    ApplyBoundaryConditions(&values);
  }

  int GridSize() const {
    return size;
  }

  T H() const {
    return h;
  }

  T Value(int i, int j) const {
    return values[Index(i, j)];
  }

  bool IsHoleNode(int i, int j) const {
    return IsHole(i, j);
  }

  bool MakeStep() override {
    nextValues = values;

    std::vector<std::future<void>> futures;
    const int blockSize = (size + numThreads - 1) / numThreads;

    for (int threadId = 0; threadId < numThreads; ++threadId) {
      const int firstRow = threadId * blockSize;
      const int lastRow = std::min(size, firstRow + blockSize);
      if (firstRow >= lastRow) {
        continue;
      }

      futures.push_back(std::async(
        std::launch::async,
        [this, firstRow, lastRow]() {
          UpdateRows(firstRow, lastRow);
        }));
    }

    for (auto& future : futures) {
      future.get();
    }

    values.swap(nextValues);
    ApplyBoundaryConditions(&values);

    return true;
  }

  /**
   * @brief Решить задачу и сохранить кадры в JSON.
   *
   * Первый кадр сохраняется до первого шага, чтобы в данных явно
   * присутствовало начальное условие u(x, y, 0) = 0 во внутренних узлах.
   *
   * @param output Выходной JSON.
   * @return true, если расчет завершился успешно.
   */
  bool Solve(nlohmann::json* output) {
    nlohmann::json& data = *output;
    int numExports = 0;

    this->t = T(0);
    data["data"][numExports]["time"] = this->t;
    ExportData(&data["data"][numExports]["data"]);
    ++numExports;

    T nextExportTime = this->exportPeriod;

    while (this->t < this->finishTime) {
      if (!MakeStep()) {
        return false;
      }

      this->t += this->tau;

      if (nextExportTime <= this->t || this->finishTime <= this->t) {
        data["data"][numExports]["time"] = this->t;
        ExportData(&data["data"][numExports]["data"]);
        ++numExports;
        nextExportTime += this->exportPeriod;
      }
    }

    return true;
  }

  void ExportData(nlohmann::json* output) override {
    (*output)["n"] = n;
    (*output)["h"] = h;
    (*output)["tau"] = this->tau;

    for (int j = 0; j < size; ++j) {
      for (int i = 0; i < size; ++i) {
        if (IsHole(i, j)) {
          (*output)["values"][j][i] = nullptr;
        } else {
          (*output)["values"][j][i] = values[Index(i, j)];
        }
      }
    }
  }

 private:
  int Index(int i, int j) const {
    return j * size + i;
  }

  T X(int i) const {
    return h * static_cast<T>(i);
  }

  T Y(int j) const {
    return h * static_cast<T>(j);
  }

  bool IsHole(int i, int j) const {
    const T x = X(i);
    const T y = Y(j);

    return x < T(1) && T(1) < y && y < T(2);
  }

  bool IsDirichletBoundary(int i, int j) const {
    const T x = X(i);
    const T y = Y(j);
    const T eps = h / T(2);

    if (std::fabs(y) < eps) {
      return true;
    }
    if (std::fabs(x - T(3)) < eps) {
      return true;
    }
    if (std::fabs(x) < eps && y <= T(1) + eps) {
      return true;
    }
    if (std::fabs(x) < eps && y >= T(2) - eps) {
      return true;
    }
    if (std::fabs(y - T(2)) < eps && x <= T(1) + eps) {
      return true;
    }
    if (std::fabs(y - T(1)) < eps && x <= T(1) + eps) {
      return true;
    }

    return false;
  }

  bool IsNeumannBoundary(int i, int j) const {
    const T x = X(i);
    const T y = Y(j);
    const T eps = h / T(2);

    if (std::fabs(y - T(3)) < eps && eps < x && x < T(3) - eps) {
      return true;
    }
    if (std::fabs(x - T(1)) < eps && T(1) + eps < y &&
        y < T(2) - eps) {
      return true;
    }

    return false;
  }

  bool IsComputationalNode(int i, int j) const {
    if (i <= 0 || j <= 0 || i >= n || j >= n) {
      return false;
    }
    if (IsHole(i, j)) {
      return false;
    }

    return !IsDirichletBoundary(i, j) && !IsNeumannBoundary(i, j);
  }

  T DirichletValue(int i, int j) const {
    const T x = X(i);
    const T y = Y(j);
    const T eps = h / T(2);

    if (std::fabs(y) < eps) {
      return T(2);
    }
    if (std::fabs(x - T(3)) < eps) {
      return T(2) - y;
    }
    if (std::fabs(x) < eps && y <= T(1) + eps) {
      return T(2) + y;
    }
    if (std::fabs(x) < eps && y >= T(2) - eps) {
      return T(-1);
    }
    if (std::fabs(y - T(2)) < eps && x <= T(1) + eps) {
      return T(-1);
    }
    if (std::fabs(y - T(1)) < eps && x <= T(1) + eps) {
      return T(3);
    }

    return T(0);
  }

  void ApplyBoundaryConditions(std::vector<T>* data) const {
    for (int j = 0; j < size; ++j) {
      for (int i = 0; i < size; ++i) {
        if (IsHole(i, j)) {
          (*data)[Index(i, j)] = std::numeric_limits<T>::quiet_NaN();
        } else if (IsDirichletBoundary(i, j)) {
          (*data)[Index(i, j)] = DirichletValue(i, j);
        }
      }
    }

    for (int i = 1; i < n; ++i) {
      (*data)[Index(i, n)] = (*data)[Index(i, n - 1)];
    }

    const int cutX = n / 3;
    const int cutYMin = n / 3;
    const int cutYMax = 2 * n / 3;
    for (int j = cutYMin + 1; j < cutYMax; ++j) {
      (*data)[Index(cutX, j)] = (*data)[Index(cutX + 1, j)];
    }
  }

  void UpdateRows(int firstRow, int lastRow) {
    const T lambda = this->tau / (h * h);

    for (int j = firstRow; j < lastRow; ++j) {
      for (int i = 0; i < size; ++i) {
        const int index = Index(i, j);

        if (!IsComputationalNode(i, j)) {
          nextValues[index] = values[index];
          continue;
        }

        nextValues[index] = values[index] + lambda * (
          values[Index(i - 1, j)] + values[Index(i + 1, j)] +
          values[Index(i, j - 1)] + values[Index(i, j + 1)] -
          T(4) * values[index]);
      }
    }
  }

  int n;
  int size;
  int numThreads;
  T h;
  std::vector<T> values;
  std::vector<T> nextValues;
};

}  // namespace mm

#endif  // INCLUDE_HEAT_EQUATION_HPP_
