/**
 * @file include/heat_equation_solver.hpp
 * @author Глеб М
 *
 * @brief Явная разностная схема для двумерного уравнения теплопроводности
 * в L‑образной области с параллелизацией через std::async.
 *
 * Подробности:
 * - Сетка равномерная по x и y: шаг h = 1 / M, индексы i (ось y), j (ось x).
 * - Область: [0,2]x[0,2] без правого верхнего квадрата (x > 1, y > 1).
 * - Граничные условия смешанные (Дирихле и Нейман), заданы согласно варианту.
 * - Уравнение: du/dt = Δu, аппроксимируется явной схемой первого порядка
 *   по времени и второго порядка по пространству.
 * - Параллелизм: на каждом шаге внутренние узлы обрабатываются несколькими
 *   потоками через std::async; граничные условия применяются последовательно
 *   после завершения всех потоков.
 */

#ifndef INCLUDE_HEAT_EQUATION_SOLVER_HPP_
#define INCLUDE_HEAT_EQUATION_SOLVER_HPP_

// Стандартные библиотеки
#include <vector>    // для хранения значений на сетке
#include <future>    // std::async, std::future -- запуск асинхронных задач
#include <thread>    // std::thread::hardware_concurrency -- число ядер

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
class HeatEquationSolver : public AbstractSolver<T> {
 public:
  /**
   * @brief Конструктор.
   *
   * @param M -- количество разбиений на единицу длины (шаг h = 1/M).
   * @param tau -- шаг по времени.
   * @param finishTime -- конечное время моделирования.
   * @param exportPeriod -- интервал сохранения данных в единицах времени.
   */
  HeatEquationSolver(size_t M, T tau, T finishTime, T exportPeriod);

  /**
   * @brief Один шаг явной разностной схемы.
   *
   * @return true, если шаг выполнен успешно.
   */
  bool MakeStep() override;

  /**
   * @brief Сохраняет текущее температурное поле в JSON.
   *
   * @param output -- указатель на JSON-объект, куда будет записана сетка.
   *
   * Важно: с _ на конце обозначаю приватные переменные (вроде так надо по правилам оформления)
   */
  void ExportData(nlohmann::json* output) override;

 private:
  // Параметры сетки M_ и h_
  size_t M_;   // число разбиений на единицу длины
  T h_;        // шаг сетки = 1.0 / M_

  // Два слоя значений (текущий и следующий) для обновления по явной схеме.
  // Хранятся как одномерные массивы размера (2M+1)*(2M+1).
  // Индексация: строка i (координата y), столбец j (координата x).
  std::vector<T> u_;       // u^n -- текущий временной слой
  std::vector<T> u_next_;  // u^{n+1} -- следующий слой

  //Вспомогательные методы

  /**
   * @brief Преобразует двумерные индексы в индекс плоского массива.
   * @param i -- номер строки (ось y: 0 .. 2M)
   * @param j -- номер столбца (ось x: 0 .. 2M)
   * @return индекс в массиве u_, оно же u_next_.
   */
  size_t Index(size_t i, size_t j) const {
    return i * (2 * M_ + 1) + j;
  }

  /**
   * @brief Проверяет, принадлежит ли узел (i,j) L‑образной области.
   *
   * Область: весь квадрат [0,2]x[0,2], за исключением точек,
   * у которых x > 1 (j > M_) и y > 1 (i > M_).
   */
  bool IsInside(size_t i, size_t j) const {
    return !(j > M_ && i > M_);
  }

  //Методы для работы с граничными условиями

  /**
   * @brief Определяет, находится ли узел (i,j) на границе Дирихле.
   *
   * Используются целочисленные индексы, поэтому сравнение точное.
   *
   *Прим для себя: тут size_t это беззнаковый целый,
   *и сравнения вида == корректны.
   */
  bool IsDirichlet(size_t i, size_t j) const;

  /**
   * @brief Определяет, находится ли узел (i,j) на границе Неймана.
   */
  bool IsNeumann(size_t i, size_t j) const;

  /**
   * @brief Вычисляет заданную температуру для узла Дирихле.
   *
   * Формулы берутся из условия задачи.
   */
  T DirichletValue(size_t i, size_t j) const;

  /**
   * @brief Заполняет массивы начальными значениями.
   *
   * Внутри области -- ноль, на границах Дирихле -- аналитическое значение.
   *
   *Прим: ну по крайней мере я так понял. Или как оно надо?
   */
  void InitializeArrays();
};


// Реализация

// Конструктор
template<typename T>
HeatEquationSolver<T>::HeatEquationSolver(
    size_t M, T tau, T finishTime, T exportPeriod)
    // Вызов конструктора базового класса AbstractSolver<T>
    : AbstractSolver<T>(tau, finishTime, exportPeriod),
      M_(M),
      h_(static_cast<T>(1.0) / static_cast<T>(M)),
      // Выделяем память под оба слоя (квадратную сетку)
      u_((2 * M + 1) * (2 * M + 1)),
      u_next_((2 * M + 1) * (2 * M + 1)) {
  // Заполняем начальные условия
  InitializeArrays();
}

// Инициализация начального поля
template<typename T>
void HeatEquationSolver<T>::InitializeArrays() {
  const size_t N = 2 * M_ + 1;  // полный размер сетки по каждому направлению
  // Обходим все узлы квадрата
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < N; ++j) {
      // Пропускаем точки вне L‑области
      if (!IsInside(i, j))
        continue;

      T value = 0;  // внутри области начальная температура = 0. (да?)
      // Если узел лежит на границе Дирихле, используем заданное значение.
      if (IsDirichlet(i, j))
        value = DirichletValue(i, j);

      // Записываем начальное значение в текущий слой
      u_[Index(i, j)] = value;
    }
  }
}

// Проверка принадлежности границе Дирихле (писано по картинке, вроде оно)
template<typename T>
bool HeatEquationSolver<T>::IsDirichlet(size_t i, size_t j) const {
  const size_t N = 2 * M_;  // максимальный индекс (2M)

  // Верхняя граница: y = 2 (i = 2M), x от 0 до 1 (j <= M)
  if (i == N && j <= M_)
    return true;
  // Правая вертикальная граница: x = 1 (j = M), y от 1 до 2 (i от M до 2M)
  if (j == M_ && i >= M_ && i <= N)
    return true;
  // Нижняя граница: y = 0 (i = 0), все x
  if (i == 0)
    return true;
  // Правая нижняя граница: x = 2 (j = 2M), y от 0 до 1 (i <= M)
  if (j == N && i <= M_)
    return true;

  return false;
}

// Проверка принадлежности границе Неймана (-||-)
template<typename T>
bool HeatEquationSolver<T>::IsNeumann(size_t i, size_t j) const {
  const size_t N = 2 * M_;

  // Левая граница: x = 0 (j = 0), исключая углы (i=0 и i=2M -- они по идее Дирихле)
  if (j == 0 && i > 0 && i < N)
    return true;
  // Внутренняя горизонтальная граница: y = 1 (i = M), x от 1 до 2
  // (строго между j=M и j=2M, исключая углы)
  if (i == M_ && j > M_ && j < N)
    return true;

  return false;
}

// Вычисление значения на границе Дирихле
template<typename T>
T HeatEquationSolver<T>::DirichletValue(size_t i, size_t j) const {
  // Преобразуем индексы в физические координаты (y, x)
  T fy = static_cast<T>(i) / static_cast<T>(M_);  
  // y = i * h
  // x = j * h

  // Верх (0,2)-(1,2): u = 2
  if (i == 2 * M_ && j <= M_)
    return 2;
  // Правая вертикаль (1,2)-(1,1): u = 4 - y
  if (j == M_ && i >= M_ && i <= 2 * M_)
    return 4 - fy;
  // Низ (2,0)-(0,0): u = 1
  if (i == 0)
    return 1;
  // Правая нижняя (2,1)-(2,0): u = 1 + y
  if (j == 2 * M_ && i <= M_)
    return 1 + fy;

  return 0;  // заглушка
}

// Один шаг по времени (явная схема + параллелизм)
template<typename T>
bool HeatEquationSolver<T>::MakeStep() {
  const T h2 = h_ * h_;              // квадрат шага сетки
  const T tau = this->tau;           // шаг по времени (из базового класса)
  const T coeff = tau / h2;          // безразмерный коэффициент (τ/h²)
  const size_t N = 2 * M_ + 1;       // количество узлов по одному направлению

  // Определяем, сколько потоков можно использовать.
  // Ограничиваем количество потоков количеством строк сетки,
  // чтобы не создавать лишние потоки, которые выйдут за пределы.
  // Прим: честно стырено с инета, вроде бы так лучше
  unsigned int nthreads = std::thread::hardware_concurrency();
  if (nthreads < 1) nthreads = 4;    // fallback
  if (nthreads > N) nthreads = static_cast<unsigned int>(N); 
  // теперь потоков не больше строк

  // Здесь будем хранить future-объекты для каждого потока
  std::vector<std::future<void>> futures;

  // Разбиваем строки (индексы i) на блоки +- равного размера
  size_t rows_per_thread = (N + nthreads - 1) / nthreads;

  for (unsigned int t = 0; t < nthreads; ++t) {
    size_t start = t * rows_per_thread;
    if (start >= N) break; // на случай, если nthreads > N, лишние потоки не запускаем
    size_t end = start + rows_per_thread;
    if (end > N) end = N;

    // Лямбда, которая будет выполнена в отдельном потоке.
    // Захватываем всё необходимое по значению (this, диапазон, коэффициенты)
    futures.push_back(std::async(std::launch::async,
      [this, start, end, coeff, N]() {
        // Обрабатываем строки с start до end (не включая end)
        for (size_t i = start; i < end; ++i) {
          for (size_t j = 0; j < N; ++j) {
            // Точки вне области и граничные пропускаем
            if (!IsInside(i, j)) continue;
            if (IsDirichlet(i, j) || IsNeumann(i, j)) continue;

            // Индекс центрального узла
            size_t idx = Index(i, j);

            // Значения в четырёх соседних узлах (старый слой u_)
            T u_im1 = u_[Index(i - 1, j)];
            T u_ip1 = u_[Index(i + 1, j)];
            T u_jm1 = u_[Index(i, j - 1)];
            T u_jp1 = u_[Index(i, j + 1)];
            // Значения в пятой точке (эта т. в этом старом слое)
            T u_ij  = u_[idx]; 

            // Явная схема: u^{n+1} =
            // u^n + (τ/h²)*(u_{i-1} + u_{i+1} + u_{j-1} + u_{j+1} - 4*u_{ij})
            // Прим: так в файлике с задачей написано
            u_next_[idx] = u_ij + coeff * (u_im1 + u_ip1 + u_jm1 + u_jp1 - 4 * u_ij);
          }
        }
      }
    ));
  }

  // Ожидаем завершения всех потоков
  for (auto& f : futures) {
    f.get();
  }

  // После того как все внутренние узлы нового слоя вычислены,
  // применяем граничные условия (уже последовательно, т.к. их тупо мало)

  // 1) Условия Дирихле -- просто присваиваем заданные значения
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < N; ++j) {
      if (!IsInside(i, j)) continue;
      if (IsDirichlet(i, j)) {
        u_next_[Index(i, j)] = DirichletValue(i, j);
      }
    }
  }

  // 2) Условия Неймана (du/dn = 0) -- аппроксимация:
  // значение на границе приравниваем значению в соседнем внутреннем узле

  // Левая граница (x = 0, кроме углов): u_{i,0} = u_{i,1}
  for (size_t i = 1; i < N - 1; ++i) {
    if (IsInside(i, 0) && IsNeumann(i, 0)) {
      u_next_[Index(i, 0)] = u_next_[Index(i, 1)];
    }
  }

  // Внутренняя горизонтальная граница (y = 1, x от 1 до 2):
  // u_{M, j} = u_{M+1, j}
  for (size_t j = M_ + 1; j < N - 1; ++j) {
    if (IsInside(M_, j) && IsNeumann(M_, j)) {
      u_next_[Index(M_, j)] = u_next_[Index(M_ + 1, j)];
    }
  }

  // Переключаем слои: новый слой становится текущим,
  // старый слой (u_) теперь будет использован для следующего шага.
  u_.swap(u_next_);

  return true;
}

// Экспорт текущего температурного поля в JSON
template<typename T>
void HeatEquationSolver<T>::ExportData(nlohmann::json* output) {
  const size_t N = 2 * M_ + 1;
  // Создаём двумерный JSON-массив (grid)
  nlohmann::json grid = nlohmann::json::array();

  for (size_t i = 0; i < N; ++i) {
    nlohmann::json row = nlohmann::json::array();
    for (size_t j = 0; j < N; ++j) {
      if (IsInside(i, j)) {
        // Точка в области -- записываем значение температуры
        row.push_back(u_[Index(i, j)]);
      } else {
        // Точка вне области -- записываем null
        row.push_back(nullptr);
      }
    }
    grid.push_back(row);
  }

  // Сохраняем сетку в выходной JSON с ключом "grid"
  (*output)["grid"] = grid;
}

}  // namespace mm -- это вот к этому

#endif  // INCLUDE_HEAT_EQUATION_SOLVER_HPP_
