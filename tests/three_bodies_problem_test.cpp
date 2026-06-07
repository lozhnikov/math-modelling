/**
 * @file tests/three-body-problem_test.hpp
 * @author Denis Kirilenko
 *
 * Тесты для задачи трех тел
 */

#include <httplib.h>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>
#include "test.hpp"
#include "test_core.hpp"

using json = nlohmann::json;

// Вспомогательная функция: ожидание завершения задачи
static bool WaitForTask(httplib::Client* cli,
  int taskId, int maxAttempts = 100) {
  for (int i = 0; i < maxAttempts; ++i) {
    char buf[64];
    snprintf(buf, sizeof(buf), R"({"id":%d})", taskId);
    auto statusRes = cli->Post("/CheckTaskStatus", buf, "application/json");
    if (!statusRes)
      continue;
    auto statusJson = json::parse(statusRes->body);
    if (statusJson["status"] == "finished")
      return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return false;
}

// Статический тест 1:

static void SimpleDoubleTest(httplib::Client* cli) {
  // Начальные условия: три тела в вершинах равностороннего треугольника,
  // нулевые скорости. Время 20.0.
  json input = {
    {"tau", 0.01},
    {"finishTime", 20.0},
    {"exportPeriod", 0.5},
    {"precision", "double"},
    {"body0", {
      {"mass", 1e8},
      {"position", {{"x", 0.0}, {"y", 5.77350269}, {"z", 0.0}}},
      {"velocity", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}}
    }},
    {"body1", {
      {"mass", 1e8},
      {"position", {{"x", -5.0}, {"y", -2.88675134}, {"z", 0.0}}},
      {"velocity", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}}
    }},
    {"body2", {
      {"mass", 1e8},
      {"position", {{"x", 5.0}, {"y", -2.88675134}, {"z", 0.0}}},
      {"velocity", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}}
    }}
  };

  // Отправка задачи
  auto res = cli->Post("/ThreeBodyProblem", input.dump(), "application/json");
  REQUIRE(res);
  REQUIRE(res->status == 200);
  json resp = json::parse(res->body);
  REQUIRE(resp.contains("id"));
  int taskId = resp["id"];

    // Ожидание завершения
  bool finished = WaitForTask(cli, taskId);
  REQUIRE(finished);

  // Скачивание результата
  char buf[64];
  snprintf(buf, sizeof(buf), R"({"id":%d})", taskId);
  auto dataRes = cli->Post("/DownloadTaskData", buf, "application/json");
  REQUIRE(dataRes);
  json result = json::parse(dataRes->body);
  REQUIRE(result["status"] == "ok");
  REQUIRE(result.contains("data"));
  REQUIRE(result["data"].is_array());
  REQUIRE(result["data"].size() > 0);
}

// Статический тест 2: круговое движение трёх тел

static void CircularOrbitTest(httplib::Client* cli) {
  json input = {
    {"tau", 0.01},
    {"finishTime", 20.0},
    {"exportPeriod", 0.5},
    {"precision", "double"},
    {"body0", {
      {"mass", 1e8},
      {"position", {{"x", 2.0}, {"y", 0.0}, {"z", 0.0}}},
      {"velocity", {{"x", 0.0}, {"y", 1.0}, {"z", 0.0}}}
    }},
    {"body1", {
      {"mass", 1e8},
      {"position", {{"x", -1.0}, {"y", 1.73205}, {"z", 0.0}}},
      {"velocity", {{"x", -0.866025}, {"y", -0.5}, {"z", 0.0}}}
    }},
    {"body2", {
      {"mass", 1e8},
      {"position", {{"x", -1.0}, {"y", -1.73205}, {"z", 0.0}}},
      {"velocity", {{"x", 0.866025}, {"y", -0.5}, {"z", 0.0}}}
    }}
  };

  auto res = cli->Post("/ThreeBodyProblem", input.dump(), "application/json");
  REQUIRE(res);
  json resp = json::parse(res->body);
  int taskId = resp["id"];

  bool finished = WaitForTask(cli, taskId);
  REQUIRE(finished);

  char buf[64];
  snprintf(buf, sizeof(buf), R"({"id":%d})", taskId);
  auto dataRes = cli->Post("/DownloadTaskData", buf, "application/json");
  REQUIRE(dataRes);
  json result = json::parse(dataRes->body);
  REQUIRE(result["status"] == "ok");
  REQUIRE(result.contains("data"));
  REQUIRE(result["data"].size() > 0);
}

// Тест со случайными начальными условиями
static void RandomInitialConditionsTest(httplib::Client* cli) {
  // Инициализируем генератор случайных чисел
  std::srand(static_cast<unsigned>(std::time(nullptr)));

  auto randomDouble = [](double min, double max) {
    return min + (max - min) * (std::rand() / static_cast<double>(RAND_MAX));
  };

  json input = {
    {"tau", 0.01},
    {"finishTime", 5.0},   // меньше, чтобы тест выполнялся быстро
    {"exportPeriod", 0.5},
    {"precision", "double"}
  };

  for (int i = 0; i < 3; ++i) {
    std::string bodyKey = "body" + std::to_string(i);
    input[bodyKey] = {
      {"mass", randomDouble(1e3, 1e9)},
      {"position", {
        {"x", randomDouble(-20.0, 20.0)},
        {"y", randomDouble(-20.0, 20.0)},
        {"z", randomDouble(-20.0, 20.0)}
      }},
      {"velocity", {
         {"x", randomDouble(-3.0, 3.0)},
         {"y", randomDouble(-3.0, 3.0)},
         {"z", randomDouble(-3.0, 3.0)}
       }}
    };
  }

  auto res = cli->Post("/ThreeBodyProblem", input.dump(), "application/json");
    REQUIRE(res);
    REQUIRE(res->status == 200);
    json resp = json::parse(res->body);
    REQUIRE(resp.contains("id"));
    int taskId = resp["id"];

    bool finished = WaitForTask(cli, taskId);
    REQUIRE(finished);

    char buf[64];
    snprintf(buf, sizeof(buf), R"({"id":%d})", taskId);
    auto dataRes = cli->Post("/DownloadTaskData", buf, "application/json");
    REQUIRE(dataRes);
    json result = json::parse(dataRes->body);
    REQUIRE(result["status"] == "ok");
    REQUIRE(result.contains("data"));
    REQUIRE(result["data"].size() > 0);
}

//  Тест для рисовалки
static void PlotTest(httplib::Client* cli) {
  // Земля-Луна-Солнце
  json input = {
    {"tau", 0.01},
    {"finishTime", 40.0},
    {"exportPeriod", 0.04},
    {"precision", "double"},
    {"body0", {
      {"mass", 1e10},
      {"position", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}},
      {"velocity", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}}
    }},
    {"body1", {
      {"mass", 1e6},
      {"position", {{"x", 10.0}, {"y", 0.0}, {"z", 0.0}}},
      {"velocity", {{"x", 0.0}, {"y", 2e-6}, {"z", 2e-6}}}
    }},
    {"body2", {
      {"mass", 1e4},
      {"position", {{"x", 10.3}, {"y", 0.3}, {"z", 0.1}}},
      {"velocity", {{"x", 0.0}, {"y", 2e-6}, {"z", 2.2e-6}}}
    }}
  };

  auto res = cli->Post("/ThreeBodyProblem", input.dump(), "application/json");
  REQUIRE(res);
  json resp = json::parse(res->body);
  int taskId = resp["id"];

  bool finished = WaitForTask(cli, taskId);
  REQUIRE(finished);

  char buf[64];
  snprintf(buf, sizeof(buf), R"({"id":%d})", taskId);
  auto dataRes = cli->Post("/DownloadTaskData", buf, "application/json");
  REQUIRE(dataRes);
  json result = json::parse(dataRes->body);
  REQUIRE(result["status"] == "ok");

  // Сохраняем данные в файл для построения картинки
  std::filesystem::path dataDir("data");
  if (!std::filesystem::exists(dataDir))
    std::filesystem::create_directory(dataDir);
  std::string jsonDataPath = (dataDir / "three_body_result.json").string();
  {
    std::ofstream fout(jsonDataPath);
    fout << result["data"].dump();  // сохраняем только массив data
  }

  std::filesystem::path pythonDir("python");
  std::string plotterPath = (pythonDir / "plot.py").string();
  std::string outputImagePath = (dataDir /
    "three_body_trajectory.png").string();

  // Формируем команду: python3 plot.py
  char command[1024];
  snprintf(command, sizeof(command),
    "python3 \"%s\" ThreeBodyPlotter \"%s\" \"%s\"",
    plotterPath.c_str(), jsonDataPath.c_str(), outputImagePath.c_str());
  int code = system(command);
  if (code != 0) {
    // Попробуем python
    snprintf(command, sizeof(command),
      "python \"%s\" ThreeBodyPlotter \"%s\" \"%s\"",
      plotterPath.c_str(), jsonDataPath.c_str(), outputImagePath.c_str());
      code = system(command);
  }
  REQUIRE_EQUAL(code, 0);
}

// Главная точка входа для тестов (вызывается из tests/main.cpp)
void TestThreeBodyProblem(httplib::Client* cli) {
  TestSuite suite("ThreeBodyProblem");
  RUN_TEST_REMOTE(suite, cli, SimpleDoubleTest);
  RUN_TEST_REMOTE(suite, cli, CircularOrbitTest);
  RUN_TEST_REMOTE(suite, cli, RandomInitialConditionsTest);
  RUN_TEST_REMOTE(suite, cli, PlotTest);
}
