/**
 * @file tests/equation_test.cpp
 * @author Энгельгардт М
 *
 * @brief Тесты для решателя уравнения теплопроводности.
 *
 * Проверяют:
 * - соблюдение граничных условий (Дирихле и Неймана) после одного шага,
 * - отсутствие NaN/Inf при длительном счёте,
 * - выполнение принципа максимума,
 * - работу через сервер (несколько задач в очереди, сохранение данных).
 */

#include <cmath>
#include <chrono>
#include <fstream>
#include <thread>
#include <string>
#include <nlohmann/json.hpp>
#include "equation_solver.hpp"
#include "test_core.hpp"
#include "test.hpp"

/**
 * @brief Проверка граничных условий после одного шага.
 */
static void TestBoundaryConditions() {
  const size_t M = 10;
  const double h = 1.0 / M;
  const double tau = h * h / 4.0;
  const double finishTime = tau;    // остановка после первого шага

  mm::EquationSolver<double> solver(M, tau, finishTime, tau);
  nlohmann::json result;
  REQUIRE(solver.Solve(&result));

  auto grid = result["data"][0]["data"]["grid"];
  REQUIRE_EQUAL(grid.size(), 3 * M + 1);

  // y = 0: u = 2
  REQUIRE_CLOSE(grid[0][0].get<double>(), 2.0, 1e-10);
  REQUIRE_CLOSE(grid[0][3 * M].get<double>(), 2.0, 1e-10);
  // y = 3, 0 <= x <= 1: u = 0
  REQUIRE(std::fabs(grid[3 * M][0].get<double>()) < 1e-12);
  REQUIRE(std::fabs(grid[3 * M][M].get<double>()) < 1e-12);
  // x = 3, y = 0.5: u = 2 + 0.5
  REQUIRE_CLOSE(grid[M / 2][3 * M].get<double>(), 2.5, 1e-10);
  // y = 1, x = 2: u = 6 - 2; угол (1, 1): u = 5
  REQUIRE_CLOSE(grid[M][2 * M].get<double>(), 4.0, 1e-10);
  REQUIRE_CLOSE(grid[M][M].get<double>(), 5.0, 1e-10);

  // Вырезанный квадрат (x > 1, y > 1) экспортируется как null
  REQUIRE(grid[2 * M][2 * M].is_null());
  REQUIRE(grid[M + 1][M + 1].is_null());
  // y = 1 при x < 1 -- внутренняя точка, а не граница Дирихле
  REQUIRE(!grid[M][M / 2].is_null());
}

/**
 * @brief Проверка условий Неймана после многих шагов.
 */
static void TestNeumann() {
  const size_t M = 8;
  const double h = 1.0 / M;
  const double tau = h * h / 4.0;

  mm::EquationSolver<double> solver(M, tau, 0.2, 1.0);
  nlohmann::json result;
  REQUIRE(solver.Solve(&result));

  auto grid = result["data"].back()["data"]["grid"];
  // x = 0: u(0, y) = u(h, y)
  for (size_t i = 1; i < 3 * M; ++i)
    REQUIRE_EQUAL(grid[i][0].get<double>(), grid[i][1].get<double>());
  // x = 1, 1 < y < 3: u(1, y) = u(1 - h, y)
  for (size_t i = M + 1; i < 3 * M; ++i)
    REQUIRE_EQUAL(grid[i][M].get<double>(), grid[i][M - 1].get<double>());
  // Тепло от нижней границы дошло до верхней части левой полосы
  REQUIRE(grid[2 * M][M / 2].get<double>() > 0.0);
}

/**
 * @brief Неустойчивый шаг по времени должен отклоняться.
 */
static void TestUnstableTau() {
  const size_t M = 10;
  const double h = 1.0 / M;
  mm::EquationSolver<double> solver(M, h * h, 0.1, 0.1);
  nlohmann::json result;
  REQUIRE(!solver.Solve(&result));
}

/**
 * @brief Проверка устойчивости: отсутствие NaN и бесконечностей.
 */
static void TestStability() {
  const size_t M = 8;
  const double h = 1.0 / M;
  const double tau = h * h / 4.0;
  const double finishTime = 0.1;

  mm::EquationSolver<double> solver(M, tau, finishTime, finishTime / 4);
  nlohmann::json result;
  REQUIRE(solver.Solve(&result));

  for (auto& frame : result["data"]) {
    for (auto& row : frame["data"]["grid"]) {
      for (auto& val : row) {
        if (!val.is_null()) {
          double v = val.get<double>();
          REQUIRE(!std::isnan(v));
          REQUIRE(!std::isinf(v));
        }
      }
    }
  }
}

/**
 * @brief Проверка принципа максимума.
 *
 * Начальное значение 0, граничные значения от 0 до 5 (угол (1,1): 6 - 1),
 * поэтому решение всё время лежит в [0, 5].
 */
static void TestMaximumPrinciple() {
  const size_t M = 6;
  const double h = 1.0 / M;
  const double tau = h * h / 4.0;
  const double finishTime = 0.5;

  mm::EquationSolver<double> solver(M, tau, finishTime, finishTime / 10);
  nlohmann::json result;
  REQUIRE(solver.Solve(&result));

  double max_val = -1e9;
  double min_val = 1e9;
  for (auto& frame : result["data"]) {
    for (auto& row : frame["data"]["grid"]) {
      for (auto& val : row) {
        if (!val.is_null()) {
          double v = val.get<double>();
          if (v > max_val) max_val = v;
          if (v < min_val) min_val = v;
        }
      }
    }
  }
  REQUIRE(max_val <= 5.0 + 1e-10);
  REQUIRE(min_val >= 0.0 - 1e-10);
}

/**
 * @brief Ждёт завершения задачи на сервере и скачивает её данные.
 */
static nlohmann::json WaitAndDownload(httplib::Client* cli, int id) {
  nlohmann::json request;
  request["id"] = id;

  // Запас по времени с учётом запуска под valgrind.
  auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes(15);
  while (std::chrono::steady_clock::now() < deadline) {
    auto res = cli->Post("/CheckTaskStatus", request.dump(),
                         "application/json");
    REQUIRE(static_cast<bool>(res));
    nlohmann::json status = nlohmann::json::parse(res->body);
    if (status["status"] == "finished") {
      auto data = cli->Post("/DownloadTaskData", request.dump(),
                            "application/json");
      REQUIRE(static_cast<bool>(data));
      return nlohmann::json::parse(data->body);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  throw std::runtime_error("Task " + std::to_string(id) + " timed out");
}

/**
 * @brief Отправляет задачу на сервер.
 */
static int PostTask(httplib::Client* cli, size_t M, double finishTime,
                    double exportPeriod) {
  const double h = 1.0 / static_cast<double>(M);
  nlohmann::json input;
  input["M"] = M;
  input["tau"] = h * h / 4.0;
  input["finishTime"] = finishTime;
  input["exportPeriod"] = exportPeriod;

  auto res = cli->Post("/HeatEquation", input.dump(), "application/json");
  REQUIRE(static_cast<bool>(res));
  return nlohmann::json::parse(res->body)["id"].get<int>();
}

/**
 * @brief Решение через сервер: две задачи в очереди одновременно,
 * результат основной задачи сохраняется в data/heat_equation.json.
 */
static void TestServer(httplib::Client* cli) {
  // Ставим сразу две задачи с разными M, чтобы проверить очередь.
  int id1 = PostTask(cli, 20, 2.0, 0.05);
  int id2 = PostTask(cli, 4, 0.1, 0.05);
  REQUIRE(id1 > 0);
  REQUIRE(id2 > 0);
  REQUIRE(id1 != id2);

  nlohmann::json out1 = WaitAndDownload(cli, id1);
  nlohmann::json out2 = WaitAndDownload(cli, id2);

  REQUIRE_EQUAL(out1["status"].get<std::string>(), std::string("ok"));
  REQUIRE_EQUAL(out2["status"].get<std::string>(), std::string("ok"));
  REQUIRE_EQUAL(out1["data"][0]["data"]["M"].get<size_t>(), size_t(20));
  REQUIRE_EQUAL(out2["data"][0]["data"]["M"].get<size_t>(), size_t(4));

  std::ofstream file("data/heat_equation.json");
  file << out1.dump();
}

/**
 * @brief Сервер должен отклонять неустойчивые параметры.
 */
static void TestServerInvalid(httplib::Client* cli) {
  nlohmann::json input;
  input["M"] = 10;
  input["tau"] = 0.01;  // > h^2/4 = 0.0025
  input["finishTime"] = 0.1;
  input["exportPeriod"] = 0.1;

  auto res = cli->Post("/HeatEquation", input.dump(), "application/json");
  REQUIRE(static_cast<bool>(res));
  REQUIRE_EQUAL(nlohmann::json::parse(res->body)["id"].get<int>(), -1);
}

void TestEquation(httplib::Client* cli) {
  TestSuite suite("Heat Equation");
  RUN_TEST(suite, TestBoundaryConditions);
  RUN_TEST(suite, TestNeumann);
  RUN_TEST(suite, TestUnstableTau);
  RUN_TEST(suite, TestStability);
  RUN_TEST(suite, TestMaximumPrinciple);
  RUN_TEST_REMOTE(suite, cli, TestServer);
  RUN_TEST_REMOTE(suite, cli, TestServerInvalid);
}
