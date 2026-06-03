#include <httplib.h>
#include <cstdio>
#include <string>

#include <filesystem>
#include <thread>
#include <nlohmann/json.hpp>

#include "test_core.hpp"

static void SimpleDoubleTest(httplib::Client* cli);
static void PlotTest(httplib::Client* cli);

void TestThreeBodySolver(httplib::Client* cli) {
  TestSuite suite("TestThreeBodySolver");
  RUN_TEST_REMOTE(suite, cli, SimpleDoubleTest);
  RUN_TEST_REMOTE(suite, cli, PlotTest);
}

static void SimpleDoubleTest(httplib::Client* cli) {
  nlohmann::json input = R"(
{
  "value_type": "double",
  "tau": 0.001,
  "finish_time": 0.5,
  "export_period": 0.1,
  "masses": [1.0, 1.0, 1.0],
  "charges": [1.0, 1.0, -1.0],
  "c": 1.0,
  "initial_positions": [[0,0,0], [1,0,0], [0.5,0.866,0]],
  "initial_velocities": [[0,0,0], [0,0.5,0], [0,-0.5,0]]
}
)"_json;

  auto res = cli->Post("/ThreeBodySolver", input.dump(), "application/json");
  REQUIRE(res);
  nlohmann::json output = nlohmann::json::parse(res->body);
  REQUIRE(output.contains("id"));
  int taskId = output["id"];

  bool finished = false;
  for (int k = 0; k < 100; ++k) {
    char buf[256];
    snprintf(buf, sizeof(buf), R"({"id":%d})", taskId);
    auto statusRes = cli->Post("/CheckTaskStatus", buf, "application/json");
    REQUIRE(statusRes);
    auto statusJson = nlohmann::json::parse(statusRes->body);
    if (statusJson["status"] == "finished") {
      finished = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  if (finished) {
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }

  char buf[256];
  snprintf(buf, sizeof(buf), R"({"id":%d})", taskId);
  auto dataRes = cli->Post("/DownloadTaskData", buf, "application/json");
  REQUIRE(dataRes);
  auto dataJson = nlohmann::json::parse(dataRes->body);
}

static void PlotTest(httplib::Client* cli) {
  nlohmann::json input = R"(
{
  "value_type": "double",
  "tau": 0.001,
  "finish_time": 0.5,
  "export_period": 0.1,
  "masses": [1.0, 1.0, 1.0],
  "charges": [1.0, 1.0, -1.0],
  "c": 1.0,
  "initial_positions": [[0,0,0], [1,0,0], [0.5,0.866,0]],
  "initial_velocities": [[0,0,0], [0,0.5,0], [0,-0.5,0]]
}
)"_json;

  auto res = cli->Post("/ThreeBodySolver", input.dump(), "application/json");
  REQUIRE(res);
  nlohmann::json output = nlohmann::json::parse(res->body);
  int taskId = output["id"];

  bool finished = false;
  for (int k = 0; k < 100; ++k) {
    char buf[256];
    snprintf(buf, sizeof(buf), R"({"id":%d})", taskId);
    auto statusRes = cli->Post("/CheckTaskStatus", buf, "application/json");
    REQUIRE(statusRes);
    auto statusJson = nlohmann::json::parse(statusRes->body);
    if (statusJson["status"] == "finished") {
      finished = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  if (finished) {
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }

  char buf[256];
  snprintf(buf, sizeof(buf), R"({"id":%d})", taskId);
  auto dataRes = cli->Post("/DownloadTaskData", buf, "application/json");
  REQUIRE(dataRes);
  auto dataJson = nlohmann::json::parse(dataRes->body);

  std::filesystem::path pythonDir("python");
  std::string plotterPath = (pythonDir / "plot.py").string();
  std::filesystem::path dataDir("data");
  std::filesystem::path outputPath = dataDir / "ThreeBodyExample";
  if (!std::filesystem::is_directory(outputPath)) {
    std::filesystem::create_directory(outputPath);
  }
  std::string jsonDataPath = (outputPath / "data.json").string();
  std::string videoOutputPath = (outputPath / "output.avi").string();
  {
    std::ofstream fout(jsonDataPath);
    fout << dataJson["data"].dump();
  }
  char command[1024];
  const char* fmt = "python \"%s\" ThreeBodyPlotter \"%s\" \"%s\"";
  snprintf(command, sizeof(command), fmt,
           plotterPath.c_str(),
           jsonDataPath.c_str(),
           videoOutputPath.c_str());
  int code = system(command);
  if (code != 0) {
    const char* fmt3 = "python3 \"%s\" ThreeBodyPlotter \"%s\" \"%s\"";
    snprintf(command, sizeof(command), fmt3,
             plotterPath.c_str(),
             jsonDataPath.c_str(),
             videoOutputPath.c_str());
    code = system(command);
  }
  REQUIRE_EQUAL(code, 0);
}
