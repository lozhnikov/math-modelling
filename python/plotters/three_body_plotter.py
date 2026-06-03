from matplotlib import pyplot as plt
import numpy as np
import matplotlib.animation as manimation
from mpl_toolkits.mplot3d import Axes3D
import os
from plotters.abstract_plotter import AbstractPlotter


class ThreeBodyPlotter(AbstractPlotter):

    def plot(self):
        frames = self.data
        if not frames:
            return

        fig = plt.figure(figsize=(8, 6))
        ax = fig.add_subplot(111, projection='3d')

        try:
            writer = manimation.FFMpegWriter(fps=25)
            writer.setup(fig, self.output_path, dpi=100)
        except FileNotFoundError:
            writer = manimation.ImageMagickWriter(fps=25)
            out_path = os.path.splitext(self.output_path)[0] + '.gif'
            writer.setup(fig, out_path, dpi=100)

        colors = ['red', 'green', 'blue']
        trajectories = [[] for _ in range(3)]

        for frame in frames:
            ax.clear()
            t = frame['time']
            positions = frame['positions']
            ax.set_title(f'Time = {t:.2f}')
            ax.set_xlim(-2, 2)
            ax.set_ylim(-2, 2)
            ax.set_zlim(-2, 2)
            ax.set_xlabel('X')
            ax.set_ylabel('Y')
            ax.set_zlabel('Z')

            for i, pos in enumerate(positions):
                x, y, z = pos
                trajectories[i].append((x, y, z))
                traj = np.array(trajectories[i])
                if len(traj) > 1:
                    ax.plot(traj[:, 0], traj[:, 1], traj[:, 2],
                            color=colors[i], alpha=0.5)
                ax.scatter(x, y, z, color=colors[i], s=80)

            writer.grab_frame()

        writer.finish()
        plt.close(fig)
