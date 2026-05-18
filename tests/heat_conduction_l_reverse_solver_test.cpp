/**
 * @file heat_conduction_l_reverse_solver_test.cpp
 * @author Zhuravlev George
 *
 * Тесты для алгоритма mm::HeatConductionLReverseSolver::MakeStep().
 */

#include <httplib.h>
#include <cstdio>
#include <unordered_set>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include "test_core.hpp"

static void SimpleDoubleTest(httplib::Client* cli);
static void PlotDoubleTest(httplib::Client* cli);
static void RandomDoubleTest(httplib::Client* cli);

void TestHeatConductionLReverseSolver(httplib::Client* cli) {
  TestSuite suite("TestHeatConductionLReverseSolver");

  RUN_TEST_REMOTE(suite, cli, SimpleDoubleTest);
  RUN_TEST_REMOTE(suite, cli, PlotDoubleTest);
  RUN_TEST_REMOTE(suite, cli, RandomDoubleTest);
}

/**
 * @brief Простейший статический тест.
 *
 * @param cli Указатель на HTTP клиент.
 */
static void SimpleDoubleTest(httplib::Client* cli) {
  nlohmann::json input = R"(
{
  "value_type": "double",
  "M": 20,
  "num_threads": 4,
  "tau": 0.00025,
  "finish_time": 1.0,
  "export_period": 0.1
}
)"_json;

  auto res = cli->Post("/HeatConductionLReverseSolver", input.dump(),
      "application/json");

  if (!res) {
    REQUIRE(false);
  }

  nlohmann::json output = nlohmann::json::parse(res->body);

  REQUIRE(output.find("id") != output.end());

  int taskId = output["id"];

  const int numTries = 100;
  bool success = false;

  for (int k = 0; k < numTries; k++) {
    char buffer[1024];
    const char* format = R"(
{
  "id": %d
}
)";
    snprintf(buffer, sizeof(buffer), format, taskId);

    auto res = cli->Post("/CheckTaskStatus", buffer,
          "application/json");

    if (!res) {
      REQUIRE(false);
    }

    nlohmann::json output = nlohmann::json::parse(res->body);

    std::string status = output.at("status");

    if (status == "finished") {
      success = true;
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  REQUIRE(success);

  {
    char buffer[1024];
    const char* format = R"(
  {
  "id": %d
  }
  )";
    snprintf(buffer, sizeof(buffer), format, taskId);

    auto res = cli->Post("/DownloadTaskData", buffer,
          "application/json");

    if (!res) {
      REQUIRE(false);
    }

    nlohmann::json output = nlohmann::json::parse(res->body);

    REQUIRE(output.at("status") == "ok");

    int numFrames = output.at("data").size();

    REQUIRE(numFrames > 0);

    auto& lastFrame = output.at("data").back();
    int M = lastFrame.at("data").at("M");
    auto& data = lastFrame.at("data").at("fn");

    REQUIRE(M > 0);

    // Упрощенная проверка принципа максимума.
    for (auto& elem : data) {
      if (elem.is_null())
        continue;
      double value = elem;
      REQUIRE(value < 8.001 && value > -0.001);
    }

    const double precision = 1e-8;

    // Проверка верхнего граничного условия
    for (int k = 0; k <= 2 * M; k++) {
      double value = data[2 * M * (2 * M + 1) + k];

      REQUIRE_CLOSE(value, 2.0, precision);
    }
  }
}



/**
 * @brief Тест рисовалки.
 *
 * @param cli Указатель на HTTP клиент.
 */
static void PlotDoubleTest(httplib::Client* cli) {
  nlohmann::json input = R"(
{
  "value_type": "double",
  "M": 20,
  "num_threads": 4,
  "tau": 0.00025,
  "finish_time": 1.0,
  "export_period": 0.1
}
)"_json;

  auto res = cli->Post("/HeatConductionLReverseSolver", input.dump(),
      "application/json");

  if (!res) {
    REQUIRE(false);
  }

  nlohmann::json output = nlohmann::json::parse(res->body);

  REQUIRE(output.find("id") != output.end());

  int taskId = output["id"];

  const int numTries = 100;
  bool success = false;

  for (int k = 0; k < numTries; k++) {
    char buffer[1024];
    const char* format = R"(
{
  "id": %d
}
)";
    snprintf(buffer, sizeof(buffer), format, taskId);

    auto res = cli->Post("/CheckTaskStatus", buffer,
          "application/json");

    if (!res) {
      REQUIRE(false);
    }

    nlohmann::json output = nlohmann::json::parse(res->body);

    std::string status = output.at("status");

    if (status == "finished") {
      success = true;
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  REQUIRE(success);

  {
    char buffer[1024];
    const char* format = R"(
  {
  "id": %d
  }
  )";
    snprintf(buffer, sizeof(buffer), format, taskId);

    auto res = cli->Post("/DownloadTaskData", buffer,
          "application/json");

    if (!res) {
      REQUIRE(false);
    }

    nlohmann::json output = nlohmann::json::parse(res->body);

    auto& data = output.at("data");

    std::filesystem::path pythonDir("python");
    std::string plotterPath = (pythonDir / "plot.py").string();

    std::filesystem::path dataDir("data");

    std::filesystem::path outputPath = dataDir / "HeatConductionLReverse";

    if (!std::filesystem::is_directory(outputPath)) {
      if (!std::filesystem::create_directory(outputPath)) {
        std::cerr << "Can't create output path '" << outputPath
                  << "'!" << std::endl;
        REQUIRE(false);
      }
    }

    std::string jsonDataPath = (outputPath / "data.json").string();
    std::string videoOutputPath = (outputPath / "output.avi").string();

    {
      std::ofstream fout(jsonDataPath);

      fout << data.dump();
    }

    char command[1024];

    snprintf(command, sizeof(buffer),
        "python \"%s\" HeatConductionLReversePlotter \"%s\" \"%s\"",
        plotterPath.c_str(), jsonDataPath.c_str(), videoOutputPath.c_str());

    int code = system(command);

    if (code != 0) {
      snprintf(command, sizeof(buffer),
          "python3 \"%s\" HeatConductionLReversePlotter \"%s\" \"%s\"",
          plotterPath.c_str(), jsonDataPath.c_str(), videoOutputPath.c_str());

      code = system(command);
    }

    REQUIRE_EQUAL(code, 0);

//    std::filesystem::remove_all(outputPath);
  }
}



/**
 * @brief Тест со случайными параметрами.
 *
 * @param cli Указатель на HTTP клиент.
 */
static void RandomDoubleTest(httplib::Client* cli) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> mDist(10, 30);
  std::uniform_int_distribution<int> threadsDist(1, 4);

  int M = mDist(gen);
  int numThreads = threadsDist(gen);
  double h = 1.0 / M;

  nlohmann::json input;
  input["value_type"] = "double";
  input["M"] = M;
  input["num_threads"] = numThreads;
  input["tau"] = h * h / 4.0;
  input["finish_time"] = 0.5;
  input["export_period"] = 0.1;

  auto res = cli->Post("/HeatConductionLReverseSolver", input.dump(),
      "application/json");

  if (!res) {
    REQUIRE(false);
  }

  nlohmann::json output = nlohmann::json::parse(res->body);

  REQUIRE(output.find("id") != output.end());
}
