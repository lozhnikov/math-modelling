from matplotlib import pyplot as plt
import numpy as np
import matplotlib.animation as manimation
from mpl_toolkits.mplot3d import Axes3D
import json
import os
from plotters.abstract_plotter import AbstractPlotter


class HeatConductionHolePlotter(AbstractPlotter):

    def plot(self):
        M = self.data[0]['data']['M']
        grid_size = self.data[0]['data'].get('grid_size', 3 * M)
        N = len(self.data)

        fig = None
        writer = None

        for Writer, output_path in (
                (manimation.FFMpegWriter, self.output_path),
                (manimation.PillowWriter,
                 os.path.splitext(self.output_path)[0] + '.gif')):
            try:
                fig = plt.figure()
                writer = Writer(fps=25)
                writer.setup(fig, output_path, 200)
                break
            except Exception:
                plt.close(fig)
                fig = None
                writer = None

        if writer is None:
            raise RuntimeError('No animation writer is available')

        x = np.linspace(0, 3, num=grid_size + 1)
        y = np.linspace(0, 3, num=grid_size + 1)

        X, Y = np.meshgrid(x, y)

        for t in range(N):
            Z = np.array(self.data[t]['data']['fn'])

            Z = Z.reshape((grid_size + 1, grid_size + 1))
            Z[M + 1:2 * M, M + 1:2 * M] = np.nan

            plt.title('Time: %.3f' % (self.data[t]['time']))

            try:
                ax = fig.add_subplot(projection='3d')

                ax.plot_surface(X, Y, Z, cmap=plt.cm.jet)
            except AttributeError:
                ax = fig.gca(projection='3d')

                ax.plot_surface(X, Y, Z, cmap=plt.cm.jet)

            writer.grab_frame()

            plt.clf()

        writer.finish()
