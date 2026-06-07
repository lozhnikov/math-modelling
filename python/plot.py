import sys
from plotters.three_body_plotter import ThreeBodyPlotter

if __name__ == '__main__':

    if len(sys.argv) < 4:
        print('Usage: plot.py <plotter> <json_data_path> <output_path>')
        raise SystemError

    plotters = {
      "ThreeBodyPlotter": ThreeBodyPlotter,
    }

    if not (sys.argv[1] in plotters):
        raise SystemError

    Plotter = plotters[sys.argv[1]]

    plotter = Plotter(sys.argv[2], sys.argv[3])

    plotter.plot()