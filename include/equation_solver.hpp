/**
 * @file include/equation_solver.hpp
 * @author Энгельгардт М
 *
 * @brief Явная разностная схема для двумерного уравнения теплопроводности
 * в L‑образной области с параллелизацией через std::thread.
 *
 * Подробности:
 * - Сетка равномерная по x и y: шаг h = 1 / M, индексы i (ось y), j (ось x),
 *   i, j = 0 .. 3M.
 * - Область: [0,3]x[0,3] без правого верхнего квадрата (x > 1, y > 1).
 * - Граничные условия:
 *     y = 0:              u = 2
 *     x = 3, 0 <= y <= 1: u = 2 + y
 *     y = 1, 1 <= x <= 3: u = 6 - x
 *     y = 3, 0 <= x <= 1: u = 0
 *     x = 0:              du/dx = 0
 *     x = 1, 1 <= y <= 3: du/dn = 0
 * - Уравнение: du/dt = Δu, аппроксимируется явной схемой первого порядка
 *   по времени и второго порядка по пространству. Схема устойчива при
 *   tau <= h^2 / 4.
 * - Параллелизм: на каждом шаге внутренние узлы обрабатываются несколькими
 *   потоками std::thread; граничные условия применяются последовательно
 *   после завершения всех потоков.
 */

#ifndef INCLUDE_EQUATION_SOLVER_HPP_
#define INCLUDE_EQUATION_SOLVER_HPP_

// Стандартные библиотеки
#include <vector>    // для хранения значений на сетке
#include <thread>    // std::thread -- потоки

// Сторонние зависимости (уже загружаются через CMake)
#include <nlohmann/json.hpp>    // работа с JSON-объектами (nlohmann::json)
#include <abstract_solver.hpp>  // базовый класс решалки mm::AbstractSolver<T>

namespace mm {

/**
 * @brief Решатель уравнения теплопроводности в L‑образной области.
 *
 * @tparam T тип данных для арифметики (float или double).
 *
 * Класс наследует AbstractSolver<T>, предоставляя реализацию чисто
 * виртуальных методов MakeStep() и ExportData(). Основной цикл по времени
 * реализован в базовом классе (метод Solve).
 */
template<typename T>
class EquationSolver : public AbstractSolver<T> {
 public:
  /**
   * @brief Конструктор.
   *
   * @param M -- количество разбиений на единицу длины (шаг h = 1/M).
   * @param tau -- шаг по времени.
   * @param finishTime -- конечное время моделирования.
   * @param exportPeriod -- интервал сохранения данных в единицах времени.
   */
  EquationSolver(size_t M, T tau, T finishTime, T exportPeriod);

  /**
   * @brief Один шаг явной разностной схемы.
   *
   * @return true, если шаг выполнен успешно; false, если параметры
   * не удовлетворяют условию устойчивости (tau > h^2/4) или M == 0.
   */
  bool MakeStep() override;

  /**
   * @brief Сохраняет текущее температурное поле в JSON.
   *
   * @param output -- указатель на JSON-объект, куда будет записана сетка.
   * Узлы вне области записываются как null.
   */
  void ExportData(nlohmann::json* output) override;

 private:
  // Параметры сетки M_ и h_
  size_t M_;   // число разбиений на единицу длины
  T h_;        // шаг сетки = 1.0 / M_

  // Два слоя значений (текущий и следующий) для обновления по явной схеме.
  // Хранятся как одномерные массивы размера (3M+1)*(3M+1).
  // Индексация: строка i (координата y), столбец j (координата x).
  std::vector<T> u_;       // u^n -- текущий временной слой
  std::vector<T> u_next_;  // u^{n+1} -- следующий слой

  /**
   * @brief Преобразует двумерные индексы в индекс плоского массива.
   * @param i -- номер строки (ось y: 0 .. 3M)
   * @param j -- номер столбца (ось x: 0 .. 3M)
   * @return индекс в массиве u_, оно же u_next_.
   */
  size_t Index(size_t i, size_t j) const {
    return i * (3 * M_ + 1) + j;
  }

  /**
   * @brief Проверяет, принадлежит ли узел (i,j) L‑образной области.
   *
   * Область: весь квадрат [0,3]x[0,3], за исключением точек,
   * у которых x > 1 (j > M_) и y > 1 (i > M_).
   */
  bool IsInside(size_t i, size_t j) const {
    return !(j > M_ && i > M_);
  }

  /**
   * @brief Определяет, находится ли узел (i,j) на границе Дирихле.
   */
  bool IsDirichlet(size_t i, size_t j) const;

  /**
   * @brief Определяет, находится ли узел (i,j) на границе Неймана.
   */
  bool IsNeumann(size_t i, size_t j) const;

  /**
   * @brief Вычисляет заданную температуру для узла Дирихле.
   */
  T DirichletValue(size_t i, size_t j) const;

  /**
   * @brief Заполняет массивы начальными значениями.
   *
   * Внутри области -- ноль, на границах Дирихле -- заданное значение.
   */
  void InitializeArrays();
};


// Реализация

template<typename T>
EquationSolver<T>::EquationSolver(
    size_t M, T tau, T finishTime, T exportPeriod)
    : AbstractSolver<T>(tau, finishTime, exportPeriod),
      M_(M),
      h_(M == 0 ? T(0) : static_cast<T>(1.0) / static_cast<T>(M)),
      // Выделяем память под оба слоя: (3M+1) узлов по каждой оси
      u_((3 * M + 1) * (3 * M + 1)),
      u_next_((3 * M + 1) * (3 * M + 1)) {
  InitializeArrays();
}

template<typename T>
void EquationSolver<T>::InitializeArrays() {
  const size_t N = 3 * M_ + 1;
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < N; ++j) {
      if (!IsInside(i, j))
        continue;

      T value = 0;  // начальная температура внутри области
      if (IsDirichlet(i, j))
        value = DirichletValue(i, j);

      u_[Index(i, j)] = value;
    }
  }
  // Начальный слой Неймана: u(0) = u(1) = 0 и так выполнено.
  u_next_ = u_;
}

template<typename T>
bool EquationSolver<T>::IsDirichlet(size_t i, size_t j) const {
  const size_t N = 3 * M_;  // индекс 3M соответствует координате 3

  // Нижняя граница: y = 0, 0 <= x <= 3
  if (i == 0) return true;
  // Правая граница: x = 3, 0 <= y <= 1
  if (j == N && i <= M_) return true;
  // Ступенька: y = 1, 1 <= x <= 3
  if (i == M_ && j >= M_) return true;
  // Верхняя граница: y = 3, 0 <= x <= 1
  if (i == N && j <= M_) return true;

  return false;
}

template<typename T>
bool EquationSolver<T>::IsNeumann(size_t i, size_t j) const {
  // Левая граница: x = 0, 0 < y < 3
  if (j == 0 && i > 0 && i < 3 * M_) return true;
  // Внутренняя вертикальная граница: x = 1, 1 < y < 3
  if (j == M_ && i > M_ && i < 3 * M_) return true;
  return false;
}

template<typename T>
T EquationSolver<T>::DirichletValue(size_t i, size_t j) const {
  T x = static_cast<T>(j) * h_;
  T y = static_cast<T>(i) * h_;

  if (i == 0) return 2;                // y = 0: u = 2
  if (i == M_ && j >= M_) return 6 - x;  // y = 1: u = 6 - x
  if (j == 3 * M_) return 2 + y;       // x = 3: u = 2 + y
  if (i == 3 * M_) return 0;           // y = 3: u = 0

  return 0;
}

template<typename T>
bool EquationSolver<T>::MakeStep() {
  if (M_ == 0)
    return false;

  const T h2 = h_ * h_;
  const T coeff = this->tau / h2;    // tau / h^2
  // Условие устойчивости явной схемы: tau / h^2 <= 1/4.
  if (coeff > static_cast<T>(0.25) * (1 + static_cast<T>(1e-6)))
    return false;

  const size_t N = 3 * M_ + 1;       // количество узлов по одному направлению

  unsigned int nthreads = std::thread::hardware_concurrency();
  if (nthreads < 1) nthreads = 4;
  if (nthreads > N) nthreads = static_cast<unsigned int>(N);

  std::vector<std::thread> threads;
  threads.reserve(nthreads);

  // Разбиваем строки (индексы i) на блоки равного размера
  size_t rows_per_thread = (N + nthreads - 1) / nthreads;

  for (unsigned int t = 0; t < nthreads; ++t) {
    size_t start = t * rows_per_thread;
    if (start >= N) break;
    size_t end = start + rows_per_thread;
    if (end > N) end = N;

    threads.emplace_back([this, start, end, coeff, N]() {
      for (size_t i = start; i < end; ++i) {
        for (size_t j = 0; j < N; ++j) {
          // Точки вне области и граничные пропускаем
          if (!IsInside(i, j)) continue;
          if (IsDirichlet(i, j) || IsNeumann(i, j)) continue;

          size_t idx = Index(i, j);
          T u_ij = u_[idx];
          T lap = u_[Index(i - 1, j)] + u_[Index(i + 1, j)] +
                  u_[Index(i, j - 1)] + u_[Index(i, j + 1)] - 4 * u_ij;

          u_next_[idx] = u_ij + coeff * lap;
        }
      }
    });
  }

  for (auto& thread : threads)
    thread.join();

  // 1) Условия Дирихле
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < N; ++j) {
      if (IsInside(i, j) && IsDirichlet(i, j))
        u_next_[Index(i, j)] = DirichletValue(i, j);
    }
  }

  // 2) Условия Неймана (du/dn = 0), аппроксимация первого порядка.
  // Левая граница (x = 0): u(0, y) = u(h, y)
  for (size_t i = 1; i < 3 * M_; ++i)
    u_next_[Index(i, 0)] = u_next_[Index(i, 1)];

  // Внутренняя вертикальная граница (x = 1, 1 < y < 3): u(1, y) = u(1-h, y)
  for (size_t i = M_ + 1; i < 3 * M_; ++i)
    u_next_[Index(i, M_)] = u_next_[Index(i, M_ - 1)];

  u_.swap(u_next_);

  return true;
}

template<typename T>
void EquationSolver<T>::ExportData(nlohmann::json* output) {
  const size_t N = 3 * M_ + 1;
  nlohmann::json grid = nlohmann::json::array();

  for (size_t i = 0; i < N; ++i) {
    nlohmann::json row = nlohmann::json::array();
    for (size_t j = 0; j < N; ++j) {
      if (IsInside(i, j))
        row.push_back(u_[Index(i, j)]);
      else
        row.push_back(nullptr);  // вырезанный квадрат
    }
    grid.push_back(row);
  }

  (*output)["grid"] = grid;
  (*output)["M"] = M_;
}

}  // namespace mm

#endif  // INCLUDE_EQUATION_SOLVER_HPP_
