/**
 * @file methods/methods.hpp
 * @author Mikhail Lozhnikov
 *
 * Объявления функий для серверной части алгоритмов. Эти функции должны
 * считать JSON, который прислал клиент, выполнить алгоритм и отправить клиенту
 * JSON с результатом работы алгоритма.
 */

#ifndef METHODS_METHODS_HPP_
#define METHODS_METHODS_HPP_

namespace mm {

/* Сюда нужно вставить объявление серверной части алгоритма. */

/**
 * @brief Парсер входных данных и инициализатор для класса
 * mm::HeatConductionReferenceExampleSolver.
 *
 * @param input Входные данные в формате JSON.
 * @param output Выходные данные в формате JSON.
 * @param tasksQueue Очередь задач.
 * @return Функция возвращает 0 в случае успеха и отрицательное число
 * если входные данные заданы некорректно.
 *
 * Функция запускает алгоритм алгоритм расчета уравнения теплопроводности,
 * используя входные данные в JSON формате. Результат также выдаётся
 * в JSON формате.
 */
int HeatConductionReferenceExampleSolverMethod(const nlohmann::json& input,
    nlohmann::json* output, TasksQueue* tasksQueue);


/* Конец вставки. */

}  // namespace mm

#endif  // METHODS_METHODS_HPP_
