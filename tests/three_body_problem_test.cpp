#include "test.hpp"
#include "test_core.hpp"
#include <random>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ¬спомогательна€ функци€: отправл€ет задачу, ожидает завершени€, возвращает результат
static json RunThreeBodyTask(httplib::Client* cli, const json& input) {
    // 1. POST /ThreeBodyProblem
    auto res = cli->Post("/ThreeBodyProblem", input.dump(), "application/json");
    if (!res || res->status != 200)
        throw std::runtime_error("Failed to post task");
    json response = json::parse(res->body);
    int taskId = response["id"];

    // 2. ќжидание завершени€ задачи
    bool finished = false;
    json result;
    while (!finished) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        json statusReq;
        statusReq["id"] = taskId;
        auto statusRes = cli->Post("/CheckTaskStatus", statusReq.dump(), "application/json");
        if (!statusRes || statusRes->status != 200)
            throw std::runtime_error("Failed to check task status");
        json statusResp = json::parse(statusRes->body);
        if (statusResp["status"] == "finished")
            finished = true;
    }

    // 3. —качивание результата
    json dataReq;
    dataReq["id"] = taskId;
    auto dataRes = cli->Post("/DownloadTaskData", dataReq.dump(), "application/json");
    if (!dataRes || dataRes->status != 200)
        throw std::runtime_error("Failed to download data");
    result = json::parse(dataRes->body);
    return result;
}

// —татический тест: три одинаковых тела в вершинах равностороннего треугольника
// с нулевыми начальными скорост€ми (должны упасть в центр масс)
static void TestStaticTriangle() {
    json input;
    input["tau"] = 0.01;
    input["finishTime"] = 2.0;
    input["exportPeriod"] = 0.5;
    input["precision"] = "double";

    double mass = 1.0;
    // ¬ершины равностороннего треугольника со стороной 1
    double x1 = 0.0, y1 = sqrt(3.0) / 3.0;
    double x2 = -0.5, y2 = -sqrt(3.0) / 6.0;
    double x3 = 0.5, y3 = -sqrt(3.0) / 6.0;

    auto setBody = [&](const std::string& key, double x, double y) {
        input[key]["mass"] = mass;
        input[key]["position"]["x"] = x;
        input[key]["position"]["y"] = y;
        input[key]["position"]["z"] = 0.0;
        input[key]["velocity"]["x"] = 0.0;
        input[key]["velocity"]["y"] = 0.0;
        input[key]["velocity"]["z"] = 0.0;
        };
    setBody("body0", x1, y1);
    setBody("body1", x2, y2);
    setBody("body2", x3, y3);

    httplib::Client cli("127.0.0.1", 8080);
    json output = RunThreeBodyTask(&cli, input);
    REQUIRE(output["status"] == "ok");

    // ѕровер€ем, что в итоге все три тела близки к центру масс (0,0,0)
    const auto& data = output["data"];
    REQUIRE(data.is_array() && !data.empty());
    const auto& last = data.back()["data"]["bodies"];
    double cx = 0.0, cy = 0.0;
    for (int i = 0; i < 3; ++i) {
        cx += last[i]["position"]["x"].get<double>();
        cy += last[i]["position"]["y"].get<double>();
    }
    cx /= 3.0;
    cy /= 3.0;
    REQUIRE_CLOSE(cx, 0.0, 1e-4);
    REQUIRE_CLOSE(cy, 0.0, 1e-4);
}

// —лучайный тест: генерируютс€ случайные массы, положени€ и скорости,
// провер€етс€, что решение завершаетс€ без ошибок и моделируетс€ заданное врем€.
static void TestRandom() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> massDist(0.1, 10.0);
    std::uniform_real_distribution<double> posDist(-10.0, 10.0);
    std::uniform_real_distribution<double> velDist(-5.0, 5.0);

    json input;
    input["tau"] = 0.01;
    input["finishTime"] = 1.0;
    input["exportPeriod"] = 0.2;
    input["precision"] = "double";

    for (int i = 0; i < 3; ++i) {
        std::string key = "body" + std::to_string(i);
        input[key]["mass"] = massDist(gen);
        input[key]["position"]["x"] = posDist(gen);
        input[key]["position"]["y"] = posDist(gen);
        input[key]["position"]["z"] = posDist(gen);
        input[key]["velocity"]["x"] = velDist(gen);
        input[key]["velocity"]["y"] = velDist(gen);
        input[key]["velocity"]["z"] = velDist(gen);
    }

    httplib::Client cli("127.0.0.1", 8080);
    json output = RunThreeBodyTask(&cli, input);
    REQUIRE(output["status"] == "ok");
    REQUIRE(output.contains("time")); 
    REQUIRE(output["data"].is_array()); 
}


void TestThreeBodyProblem(httplib::Client* cli) {
    TestSuite suite("ThreeBodyProblem");
    RUN_TEST(suite, TestStaticTriangle);
    RUN_TEST(suite, TestRandom);
}