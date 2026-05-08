from matplotlib import pyplot as plt
import numpy as np
import matplotlib.animation as manimation
from mpl_toolkits.mplot3d import Axes3D
import json
from plotters.abstract_plotter import AbstractPlotter


class HeatConductionReferenceExamplePlotter(AbstractPlotter):

    def plot(self):
        M = self.data[0]['data']['M']
        N = len(self.data)

        fig = plt.figure()

        writer = manimation.FFMpegWriter(fps=25)

        writer.setup(fig, self.output_path, 200)

        x = np.linspace(0, 1, num=M + 1)
        y = np.linspace(0, 1, num=M + 1)

        X, Y = np.meshgrid(x, y)

        for t in range(N):
            Z = np.array(self.data[t]['data']['fn'])

            Z = Z.reshape((M + 1, M + 1))

            plt.title(f'Time: {self.data[t]['time']}')

            ax = fig.add_subplot(projection='3d')

            ax.plot_surface(X, Y, Z, cmap=plt.cm.jet)

            writer.grab_frame()

            plt.clf()
