/** @file include/heat_equation.hpp */
#ifndef INCLUDE_HEAT_EQUATION_HPP_
#define INCLUDE_HEAT_EQUATION_HPP_

#include <algorithm>
#include <cmath>
#include <future>
#include <stdexcept>
#include <vector>

namespace mm {

/** Explicit finite differences for a notched square, with parallel rows. */
template<typename T>
class HeatEquation {
 public:
  HeatEquation(int m, T tau, T initialTemperature = T(0))
      : m_(m), n_(3 * m), h_(T(1) / m), tau_(tau),
        current_((n_ + 1) * (n_ + 1), initialTemperature),
        next_(current_) {
    if (m < 2 || !(tau > T(0)) ||
        tau > h_ * h_ / T(4)) {
      throw std::invalid_argument("Invalid grid or unstable time step");
    }
    for (int i = 0; i <= n_; ++i) {
      for (int j = 0; j <= n_; ++j) {
        if (!Inside(i, j)) {
          current_[Index(i, j)] = T(0);
        } else if (Fixed(i, j)) {
          current_[Index(i, j)] = Boundary(i, j);
        }
      }
    }
    next_ = current_;
  }

  int GridSize() const { return n_; }
  T StepSize() const { return h_; }
  T TimeStep() const { return tau_; }
  bool Inside(int i, int j) const {
    return i >= 0 && i <= n_ && j >= 0 && j <= n_ &&
           !(i < m_ && j > m_ && j < 2 * m_);
  }
  T Value(int i, int j) const { return current_.at(Index(i, j)); }

  /** Each half of the rows is computed on a separate async task. */
  void MakeStep() {
    auto first = std::async(std::launch::async, [this]() {
      UpdateRows(0, n_ / 2 + 1);
    });
    auto second = std::async(std::launch::async, [this]() {
      UpdateRows(n_ / 2 + 1, n_ + 1);
    });
    first.get();
    second.get();
    current_.swap(next_);
  }

  std::vector<std::vector<T>> Snapshot() const {
    std::vector<std::vector<T>> result(
        n_ + 1, std::vector<T>(n_ + 1));
    for (int i = 0; i <= n_; ++i) {
      for (int j = 0; j <= n_; ++j) {
        result[i][j] = Inside(i, j) ? Value(i, j) : T(0);
      }
    }
    return result;
  }

 private:
  int m_;
  int n_;
  T h_;
  T tau_;
  std::vector<T> current_;
  std::vector<T> next_;

  int Index(int i, int j) const { return i * (n_ + 1) + j; }

  bool Fixed(int i, int j) const {
    return i == n_ || j == 0 || j == n_ ||
           (i == m_ && j >= m_ && j <= 2 * m_) ||
           (j == m_ && i <= m_) ||
           (i == 0 && j >= 2 * m_);
  }

  T Boundary(int i, int j) const {
    const T x = j * h_;
    const T y = i * h_;
    if (i == n_) return x + T(3);
    if (j == 0) return y;
    if (j == n_) return T(9) - y;
    if (i == m_ && j >= m_ && j <= 2 * m_) return T(-1);
    if (j == m_ && i <= m_) return -y;
    if (i == 0 && j >= 2 * m_) return T(3) * x;
    return T(0);
  }

  void UpdateRows(int from, int to) {
    const T r = tau_ / (h_ * h_);
    for (int i = from; i < to; ++i) {
      for (int j = 0; j <= n_; ++j) {
        const int k = Index(i, j);
        if (!Inside(i, j) || Fixed(i, j)) {
          next_[k] = current_[k];
          continue;
        }
        const T center = current_[k];
        // Neumann: reflect the missing neighbor across the boundary.
        const T down = i == 0 ? current_[Index(i + 1, j)]
                              : current_[Index(i - 1, j)];
        const T up = current_[Index(i + 1, j)];
        const T left = (j == 2 * m_ && i < m_)
            ? current_[Index(i, j + 1)] : current_[Index(i, j - 1)];
        const T right = current_[Index(i, j + 1)];
        next_[k] = center + r * (up + down + left + right - T(4) * center);
      }
    }
  }
};

}  // namespace mm
#endif  // INCLUDE_HEAT_EQUATION_HPP_
