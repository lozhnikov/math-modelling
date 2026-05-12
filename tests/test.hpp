/**
 * @file tests/test.hpp
 * @author Mikhail Lozhnikov
 *
 * Заголовочный файл для объявлений основных тестовых функций.
 */

#ifndef TESTS_TEST_HPP_
#define TESTS_TEST_HPP_

#include <httplib.h>

/* Сюда нужно добавить объявления тестовых функций. */

/**
 * @brief Набор тестов для класса mm::HeatConductionReferenceExampleSolver.
 * @param cli Указатель на HTTP клиент.
 */
void TestHeatConductionReferenceExampleSolver(httplib::Client* cli);

/* Конец вставки. */

#endif  // TESTS_TEST_HPP_
