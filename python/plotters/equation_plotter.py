"""
Визуализатор температурного поля для задачи теплопроводности.
Сохраняет анимацию с распределением температуры в виде контурных линий.
"""

import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt  # noqa: E402
import matplotlib.animation as animation  # noqa: E402
from abstract_plotter import AbstractPlotter  # noqa: E402

# Граница L-области: [0,3]x[0,1] U [0,1]x[1,3]
BOUNDARY_X = [0, 3, 3, 1, 1, 0, 0]
BOUNDARY_Y = [0, 0, 1, 1, 3, 3, 0]


class EquationPlotter(AbstractPlotter):
    def plot(self):
        frames = self.data["data"]  # список временных слоёв
        if not frames:
            return

        # Преобразуем JSON-данные в numpy-массивы (null -> np.nan)
        grids = []
        times = []
        for fr in frames:
            times.append(fr["time"])
            grid = np.array([
                [np.nan if val is None else val for val in row]
                for row in fr["data"]["grid"]
            ], dtype=float)
            grids.append(np.ma.masked_invalid(grid))

        n = grids[0].shape[0]
        x = np.linspace(0, 3, n)
        y = np.linspace(0, 3, n)
        X, Y = np.meshgrid(x, y)

        # Фиксированные уровни для всех кадров
        vmin = min(float(g.min()) for g in grids)
        vmax = max(float(g.max()) for g in grids)
        levels = np.linspace(vmin, vmax, 21)

        fig, ax = plt.subplots(figsize=(6, 6))
        norm = matplotlib.colors.Normalize(vmin=vmin, vmax=vmax)
        mappable = matplotlib.cm.ScalarMappable(norm=norm, cmap='jet')
        fig.colorbar(mappable, ax=ax, label='u')

        def animate(i):
            ax.clear()
            cont = ax.contour(X, Y, grids[i], levels=levels, cmap='jet',
                              norm=norm, linewidths=1.2)
            ax.clabel(cont, levels[::4], inline=True, fontsize=7, fmt='%.1f')
            ax.plot(BOUNDARY_X, BOUNDARY_Y, color='black', linewidth=1.5)
            ax.set_xlim(-0.05, 3.05)
            ax.set_ylim(-0.05, 3.05)
            ax.set_aspect('equal')
            ax.set_title(f'Temperature, t = {times[i]:.4f}')
            ax.set_xlabel('x')
            ax.set_ylabel('y')
            return cont,

        ani = animation.FuncAnimation(fig, animate, frames=len(grids),
                                      interval=200, blit=False)

        if self.output_path.endswith('.gif') or \
                not animation.writers.is_available('ffmpeg'):
            ani.save(self.output_path, writer='pillow', fps=5)
        else:
            ani.save(self.output_path, writer='ffmpeg', fps=5)
        plt.close(fig)
