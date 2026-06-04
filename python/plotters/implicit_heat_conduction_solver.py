import os
import numpy as np
import matplotlib.pyplot as plt


class ImplicitHeatConductionSolverPlotter:
    """
    Класс для визуализации результатов неявной схемы теплопроводности.
    """

    def __init__(self):
        pass

    def plot(self, data_json, output_dir):
        """
        Строит двумерную тепловую карту распределения температуры.
        """
        if "M" not in data_json or "fn" not in data_json:
            print(" Неверный формат данных JSON для визуализации")
            return

        M = data_json["M"]
        fn = data_json["fn"]

        # Восстанавливаем двумерную матрицу (M + 1) x (M + 1) из одномерного
        # массива
        grid_size = M + 1
        u_matrix = np.array(fn).reshape((grid_size, grid_size))

        # Создаем пространственную сетку координат от 0 до 1
        x = np.linspace(0, 1, grid_size)
        y = np.linspace(0, 1, grid_size)
        X, Y = np.meshgrid(x, y)

        # Отрисовка тепловой карты
        plt.figure(figsize=(7, 6))
        mesh = plt.pcolormesh(X, Y, u_matrix, cmap="plasma", shading="auto")

        # Настройка цветовой шкалы и подписей
        plt.colorbar(mesh, label="Температура u(x, y)")
        plt.title("Распределение тепла (Неявная схема Якоби)")
        plt.xlabel("Координата X")
        plt.ylabel("Координата Y")
        plt.grid(True, linestyle="--", alpha=0.5)

        # Сохранение графика в папку вывода
        os.makedirs(output_dir, exist_ok=True)
        output_path = os.path.join(
            output_dir, "implicit_heat_conduction.png")
        plt.savefig(output_path, dpi=150, bbox_inches="tight")
        plt.close()

        print(f" График успешно сохранен: {output_path}")
