/**
 * @file heat_conduction_solver_test.cpp
 * @author Mikhail Lozhnikov
 *
 * Тесты для алгоритма mm::HeatConductionSolver::MakeStep().
 */

#include <httplib.h>
#include <cstdio>
#include <unordered_set>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include "test_core.hpp"

static void SimpleDoubleTest(httplib::Client* cli);
static void PlotDoubleTest(httplib::Client* cli);

void TestHeatConductionSolver(httplib::Client* cli) {
  TestSuite suite("TestHeatConductionSolver");

  RUN_TEST_REMOTE(suite, cli, SimpleDoubleTest);
  RUN_TEST_REMOTE(suite, cli, PlotDoubleTest);
}

/**
 * @brief Простейший статический тест.
 *
 * @param cli Указатель на HTTP клиент.
 */
static void SimpleDoubleTest(httplib::Client* cli) {
  /*
  Библиотека nlohmann json позволяет преобразовать
  строку в объект nlohmann::json не только при помощи
  функции nlohmann::json::parse(), но и при помощи
  специального литерала _json. Если его поставить после строки
  в кавычках, то она конвертируется в json объект.

  R"(
  )" Так записывается строка, содержащая символы перевода строки
  в C++. Всё, что между скобками это символы строки. Перводы строк
  можно ставить просто как перевод строки в текстовом редактора
  (а не через \n).
  */

  nlohmann::json input = R"(
{
  "value_type": "double",
  "M": 20,
  "tau": 0.00025,
  "finish_time": 1.0,
  "export_period": 0.1
}
)"_json;


  /* Делаем POST запрос по адресу нашего метода на сервере.
  Метод dump() используется для преобразования JSON обратно в строку.
  (Можно было сразу строку передать). При передаче JSON данных
  необходимо поставить тип MIME "application/json".
  */
  auto res = cli->Post("/HeatConductionSolver", input.dump(),
      "application/json");

  if (!res) {
    REQUIRE(false);
  }

  /* Используем метод parse() для преобразования строки ответа сервера
  (res->body) в объект JSON. */
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

int N = 3 * M;

const double precision = 1e-6;

// Проверка принципа максимума
for (double value : data) {
  REQUIRE(value >= -1.001);
  REQUIRE(value <= 4.001);
}

// Нижняя граница y = 0, u = 1
for (int j = 0; j <= N; j++) {
  double value = data[j];

  REQUIRE_CLOSE(
      value,
      1.0,
      precision);
}

// Правая граница x = 3, u = y + 1
for (int i = 0; i <= N; i++) {

  double y =
      static_cast<double>(i) / M;

  double value =
      data[i * (N + 1) + N];

  REQUIRE_CLOSE(
      value,
      y + 1.0,
      precision);
}

// Левая внутренняя граница x = 1, y ∈ [2,3]
for (int i = 2 * M; i <= N; i++) {

  double value =
      data[i * (N + 1) + M];

  std::cout
      << "i = "
      << i
      << " value = "
      << value
      << std::endl;

  REQUIRE_CLOSE(
      value,
      -1.0,
      precision);
}

// Верхняя граница y = 3, x ∈ [0,1]
for (int j = 0; j <= M; j++) {

  double x =
      static_cast<double>(j) / M;

  double value =
      data[N * (N + 1) + j];

  REQUIRE_CLOSE(
      value,
      -x,
      precision);
}
  }
}



/**
 * @brief Тест рисовалки.
 *
 * @param cli Указатель на HTTP клиент.
 */
static void PlotDoubleTest(httplib::Client* cli) {
  /*
  Библиотека nlohmann json позволяет преобразовать
  строку в объект nlohmann::json не только при помощи
  функции nlohmann::json::parse(), но и при помощи
  специального литерала _json. Если его поставить после строки
  в кавычках, то она конвертируется в json объект.

  R"(
  )" Так записывается строка, содержащая символы перевода строки
  в C++. Всё, что между скобками это символы строки. Перводы строк
  можно ставить просто как перевод строки в текстовом редактора
  (а не через \n).
  */

  nlohmann::json input = R"(
{
  "value_type": "double",
  "M": 20,
  "tau": 0.00025,
  "finish_time": 1.0,
  "export_period": 0.1
}
)"_json;


  /* Делаем POST запрос по адресу нашего метода на сервере.
  Метод dump() используется для преобразования JSON обратно в строку.
  (Можно было сразу строку передать). При передаче JSON данных
  необходимо поставить тип MIME "application/json".
  */
  auto res = cli->Post("/HeatConductionSolver", input.dump(),
      "application/json");

  if (!res) {
    REQUIRE(false);
  }

  /* Используем метод parse() для преобразования строки ответа сервера
  (res->body) в объект JSON. */
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

    std::filesystem::path pythonDir("../python");
    std::string plotterPath = (pythonDir / "plot.py").string();

    std::filesystem::path dataDir("data");

    std::filesystem::path outputPath = dataDir /
        "HeatConductionReferenceExample";

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

    

//    std::filesystem::remove_all(outputPath);
  }
}
