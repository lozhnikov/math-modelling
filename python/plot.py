import os
import sys
import json

from plotters.implicit_heat_conduction_solver import (
    ImplicitHeatConductionSolverPlotter,
)


def main():
    if len(sys.argv) < 2:
        print("Использование: python plot.py <путь_к_json_файлу_ответа>")
        return

    json_path = sys.argv[1]
    if not os.path.exists(json_path):
        print(f"Файл {json_path} не найден")
        return

    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    # Определяем тип алгоритма (извлекается из структуры данных или аргументов)
    algorithm_type = data.get(
        "algorithm_type", "ImplicitHeatConductionSolver")
    output_directory = "output"

    if algorithm_type == "ImplicitHeatConductionSolver":
        # Вызов созданного нами плоттера
        plotter = ImplicitHeatConductionSolverPlotter()
        plotter.plot(data, output_directory)

    else:
        print(f"[WARNING] Неизвестный тип алгоритма: {algorithm_type}")


if __name__ == "__main__":
    main()
