/**
 * @file tasks_queue.hpp
 * @file methods/tasks_queue.hpp
 * @author Mikhail Lozhnikov
 *
 * Класс очереди задач.
 * Очередь задач для серверной части.
 */

#ifndef METHODS_TASKS_QUEUE_HPP_
#define METHODS_TASKS_QUEUE_HPP_

#include <queue>
#include <utility>
#include <thread>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <abstract_solver_wrapper.hpp>
#include "abstract_solver_wrapper.hpp"

namespace mm {

/**
 * @brief Класс очереди задач.
 */
class TasksQueue {
 private:
  //! Очередь задач.
  std::queue<std::pair<int, AbstractSolverWrapper*>> tasks;
  //! Данные завершенныхзадач.
  std::unordered_map<int, nlohmann::json> finishedTasksData;
  //! Идентификатор последней задачи.
  int lastTaskId;
  //! Индикатор завершения очереди обработки задач.
  bool finished;
  //! Мьютекс для внутренних структур.
  std::mutex m;
  //! Условная переменная для начала обработки новой задачи.
  std::condition_variable condvar;
  //! Описатель потока - обработчика задач.
  std::thread queueThread;

 public:
  /**
   * @brief Конструктор очереди задач.
   *
   * Запускает поток-обработчик очереди задач.
   */
  TasksQueue();

  /**
   * @brief Деструктор.
   *
   * Останавливает поток-обработчик очереди задач.
   */
  ~TasksQueue();

  /**
   * @brief Добавить решалку в очередь задач.
   * @param task Решалка, добавляемая в очередь.
   * @return Идентификатор добавленной задачи.
   *
   * Функция добавляет задачу в очередь задач и присваивает ей уникальный
   * идентификатор. 
   */
  int AddTask(AbstractSolverWrapper* task);

  /**
   * @brief Проверить, завершена ли задача.
   * @param id Идентификатор задачи.
   * @return Функция возвращает true, если задача завершена, и false в противном
   * случае.
   */
  bool IsTaskFinished(int id);

   /**
    * @brief Добавить решалку в очередь задач.
    * @param wrapper Решалка, добавляемая в очередь.
    * @return Идентификатор добавленной задачи.
    *
    * Функция добавляет задачу в очередь задач и присваивает ей уникальный
    * идентификатор. 
    */
  template<typename Wrapper>
  int AddTask(Wrapper* wrapper) {
    std::lock_guard<std::mutex> lock(mutex_);

    int id = nextId_++;

    tasks_[id] = std::unique_ptr<AbstractSolverWrapper>(wrapper);
    statuses_[id] = "running";

    condition_.notify_one();

    return id;
  }

  nlohmann::json GetFinishedTaskData(int id);

 private:
  /**
   * @brief Поток, в котором выполняется очередь задач.
   */
  void ThreadFunction();
  void ProcessTasks();

  int nextId_;
  std::atomic<bool> stop_;
  std::thread worker_;

  std::mutex mutex_;
  std::condition_variable condition_;

  std::map<int, std::unique_ptr<AbstractSolverWrapper>> tasks_;
  std::map<int, std::string> statuses_;
  std::map<int, nlohmann::json> finishedData_;
};

}  // namespace mm

#endif  // METHODS_TASKS_QUEUE_HPP_
