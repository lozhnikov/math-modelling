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

#include <nlohmann/json.hpp>
namespace mm {

/* Сюда нужно вставить объявление серверной части алгоритма. */

int ThreeBodyProblemMethod(const nlohmann::json& input, nlohmann::json* output);

/* Конец вставки. */

}  // namespace mm

#endif  // METHODS_METHODS_HPP_
