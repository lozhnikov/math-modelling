import numpy as np
from matplotlib import animation
from matplotlib import pyplot as plt

from plotters.abstract_plotter import AbstractPlotter


class HeatEquationPlotter(AbstractPlotter):
    def plot(self):
        frames = self.data['data']
        values = []
        times = []

        for frame in frames:
            times.append(frame['time'])
            values.append(self._frame_to_array(frame['data']['values']))

        values = np.array(values)
        first = frames[0]['data']
        h = first['h']
        n = first['n']

        x = np.linspace(0.0, h * n, n + 1)
        y = np.linspace(0.0, h * n, n + 1)
        x_grid, y_grid = np.meshgrid(x, y)

        z_min = np.nanmin(values)
        z_max = np.nanmax(values)

        figure = plt.figure()
        axes = figure.add_subplot(111, projection='3d')

        def draw(frame_id):
            axes.clear()
            axes.plot_surface(
                x_grid,
                y_grid,
                values[frame_id],
                cmap='inferno',
                edgecolor='none',
                linewidth=0,
                antialiased=True)

            axes.set_xlim(0.0, 3.0)
            axes.set_ylim(0.0, 3.0)
            axes.set_zlim(z_min, z_max)
            axes.set_xlabel('x')
            axes.set_ylabel('y')
            axes.set_zlabel('u')
            axes.set_title('t = {:.4f}'.format(times[frame_id]))

        result = animation.FuncAnimation(
            figure,
            draw,
            frames=len(values),
            interval=80,
            repeat=True)

        if self.output_path.endswith('.html'):
            with open(self.output_path, 'w') as output:
                output.write(result.to_jshtml())
        else:
            result.save(self.output_path)

    def _frame_to_array(self, values):
        return np.array([
            [np.nan if value is None else value for value in row]
            for row in values
        ])
