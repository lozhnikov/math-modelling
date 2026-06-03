import sys
import json
from plotters.three_body_plotter import ThreeBodyPlotter

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("Usage: python plot.py ThreeBodyPlotter input.json output.mp4")
        sys.exit(1)

    plotter_name = sys.argv[1]
    input_json_path = sys.argv[2]
    output_video_path = sys.argv[3]

    # Загружаем данные
    with open(input_json_path, 'r') as f:
        data = json.load(f)

    # В данных от сервера поле "data" содержит массив кадров
    if "data" in data:
        frames = data["data"]
    else:
        frames = data   # на случай, если передан уже очищенный массив

    plotters = {
        "ThreeBodyPlotter": ThreeBodyPlotter
    }

    if plotter_name not in plotters:
        raise SystemError(f"Unknown plotter: {plotter_name}")

    Plotter = plotters[plotter_name]
    # Конструктор AbstractPlotter ожидает (input_path, output_path)
    # или (data, output_path) – здесь мы передаём данные напрямую.
    # Для совместимости создадим экземпляр, передав вместо пути данные.
    plotter = Plotter(input_json_path, output_video_path)
    plotter.data = frames   # подменяем загруженные данные
    plotter.plot()
