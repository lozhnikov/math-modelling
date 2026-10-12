
"""Animated heat surface for a square with a rectangular notch."""

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, FFMpegWriter, PillowWriter
import numpy as np
from .abstract_plotter import AbstractPlotter


class HeatEquationPlotter(AbstractPlotter):
    def plot(self):
        m = int(self.data['m'])
        h = float(self.data['h'])
        frames = self.data['data']

        x, y = np.meshgrid(np.arange(3 * m + 1) * h,
                           np.arange(3 * m + 1) * h)

        notch = (y < 1) & (x > 1) & (x < 2)

        fig = plt.figure(figsize=(8, 6))
        ax = fig.add_subplot(111, projection='3d')

        values = [np.asarray(frame['data'], dtype=float) for frame in frames]

        lo = min(np.min(v[~notch]) for v in values)
        hi = max(np.max(v[~notch]) for v in values)

        def draw(k):
            ax.clear()
            z = np.ma.masked_where(notch, values[k])

            ax.plot_surface(x, y, z, cmap='coolwarm', vmin=lo, vmax=hi)

            ax.set(xlabel='x', ylabel='y', zlabel='u',
                   title=f"Heat equation, t={frames[k]['time']:.4f}")

            ax.set_zlim(lo - 0.1, hi + 0.1)

        animation = FuncAnimation(fig, draw, frames=len(frames),
                                  interval=100, repeat=False)

        if self.output_path.lower().endswith('.gif'):
            animation.save(self.output_path, writer=PillowWriter(fps=10))
        else:
            animation.save(self.output_path, writer=FFMpegWriter(fps=10))

        plt.close(fig)
