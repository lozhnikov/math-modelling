from matplotlib import pyplot as plt
import numpy as np
import matplotlib.animation as manimation
from mpl_toolkits.mplot3d import Axes3D
import json
import os
from plotters.abstract_plotter import AbstractPlotter


class HeatConductionLReversePlotter(AbstractPlotter):

    def plot(self):
        M = self.data[0]['data']['M']
        N = len(self.data)

        fig = None
        writer = None

        try:
            fig = plt.figure()

            writer = manimation.FFMpegWriter(fps=25)

            writer.setup(fig, self.output_path, 200)
        except FileNotFoundError:
            fig = plt.figure()

            writer = manimation.ImageMagickWriter(fps=25)

            output_path = os.path.splitext(self.output_path)[0] + '.gif'

            writer.setup(fig, output_path, 200)

        x = np.linspace(0, 2, num=2 * M + 1)
        y = np.linspace(0, 2, num=2 * M + 1)

        X, Y = np.meshgrid(x, y)

        for t in range(N):
            Z = np.array(self.data[t]['data']['fn'], dtype=float)

            Z = Z.reshape((2 * M + 1, 2 * M + 1))

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
