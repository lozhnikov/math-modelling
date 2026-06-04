/**
 * @file tests/implicit_heat_conduction_solver_test.cpp
 * @author BorisKlimov
 * @brief 1D implicit heat conduction solver tests.
 */

#include <httplib.h>
#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <cmath>
#include "test.hpp"
#include "test_core.hpp"

/**
 * @brief Интеграционный тест для проверки решателя неявной теплопроводности.
 * 
 * Тест выполняет полный цикл проверки REST API сервера:
 * 1. Отправляет JSON-запрос на запуск расчета одномерной теплопроводности.
 * 2. Опрашивает сервер по ID задачи, ожидая статус окончания расчета (`finished`).
 * 3. Скачивает финальные данные физического поля.
 * 4. Верифицирует результаты (граничные условия и принцип максимума).
 * 
 * @param[in,out] cli Указатель на HTTP-клиент для отправки запросов к серверу.
 * 
 * @pre Сервер должен быть запущен и доступен по сети.
 * 
 * @throws Намеренно не выбрасывает исключений, а использует макросы макросы 
 *         тестирования `REQUIRE` и `REQUIRE_CLOSE` для фиксации падений.
 */
void TestImplicitHeatConductionSolver(httplib::Client* cli) {
  // 1. Send task to server (1D parameters)
  nlohmann::json input = R"(
{
  "value_type": "double",
  "M": 50,
  "tau": 0.0001,
  "finish_time": 0.01,
  "export_period": 0.002
}
)"_json;

  auto res = cli->Post(
    "/ImplicitHeatConductionSolver",
    input.dump(),
    "application/json"
  );

  if (!res) {
    REQUIRE(false);
  }

  nlohmann::json output = nlohmann::json::parse(res->body);
  REQUIRE(output.find("id") != output.end());

  int taskId = output["id"];

  // 2. Wait for task completion
  const int numTries = 200;
  bool success = false;

  for (int k = 0; k < numTries; ++k) {
    char buffer[1024];
    const char* format = R"({"id": %d})";
    snprintf(buffer, sizeof(buffer), format, taskId);

    auto statusRes = cli->Post(
      "/CheckTaskStatus",
      buffer,
      "application/json"
    );

    if (!statusRes) {
      REQUIRE(false);
    }

    nlohmann::json statusOutput = nlohmann::json::parse(statusRes->body);
    std::string status = statusOutput.at("status");

    if (status == "finished") {
      success = true;
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  REQUIRE(success);

  // 3. Download and check results
  {
    char buffer[1024];
    const char* format = R"({"id": %d})";
    snprintf(buffer, sizeof(buffer), format, taskId);

    auto dataRes = cli->Post(
      "/DownloadTaskData",
      buffer,
      "application/json"
    );

    if (!dataRes) {
      REQUIRE(false);
    }

    nlohmann::json dataOutput = nlohmann::json::parse(dataRes->body);
    REQUIRE(dataOutput.at("status") == "ok");

    int numFrames = dataOutput.at("data").size();
    REQUIRE(numFrames > 0);

    auto& lastFrame = dataOutput.at("data").back();
    int M = lastFrame.at("data").at("M");
    auto& data = lastFrame.at("data").at("fn");

    REQUIRE(M > 0);
    REQUIRE(data.size() == static_cast<size_t>((M + 1) * (M + 1)));

    // Check boundary conditions and maximum principle
    const double precision = 1e-6;

    // Left boundary (x = 0): u = -1.0
    double valueLeft = data[0];
    REQUIRE_CLOSE(valueLeft, -1.0, precision);

    // Right boundary (x = 1): u = -1.0
    double valueRight = data[M - 1];
    REQUIRE_CLOSE(valueRight, -1.0, precision);

    // Maximum principle check
    for (double value : data) {
      REQUIRE(value <= 0.001 && value >= -1.001);
    }
  }
}
