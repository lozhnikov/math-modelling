/**
 * @file tests/test.hpp
 * @author Mikhail Lozhnikov
 *
 * Заголовочный файл для объявлений основных тестовых функций.
 */

#ifndef TESTS_TEST_HPP_
#define TESTS_TEST_HPP_

#include <httplib.h>

/**
 * @brief Набор тестов для класса mm::HeatConductionSolver.
 * @param cli Указатель на HTTP клиент.
 */
void TestHeatConductionSolver(httplib::Client* cli);

#endif  // TESTS_TEST_HPP_
