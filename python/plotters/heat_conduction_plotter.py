from matplotlib import pyplot as plt
import numpy as np
import matplotlib.animation as manimation
from plotters.abstract_plotter import AbstractPlotter


class HeatConductionPlotter(AbstractPlotter):

    def plot(self):

        M = self.data[0]['data']['M']
        frames = len(self.data)

        N = 3 * M + 1

        fig, ax = plt.subplots(figsize=(8, 8))

        writer = manimation.FFMpegWriter(fps=25)

        writer.setup(fig, self.output_path, 200)

        x = np.linspace(0, 3, N)
        y = np.linspace(0, 3, N)

        X, Y = np.meshgrid(x, y)

        for k in range(frames):

            Z = np.array(
                self.data[k]['data']['fn'],
                dtype=float
            )

            Z = Z.reshape((N, N))

            # Вырезанный квадрат [1,2] × [2,3]
            mask = (
                (X >= 1.0) &
                (X <= 2.0) &
                (Y >= 2.0) &
                (Y <= 3.0)
            )

            Z = np.ma.array(Z, mask=mask)

            ax.clear()

            contour = ax.contourf(
                X,
                Y,
                Z,
                levels=20,
                cmap=plt.cm.jet
            )

            ax.contour(
                X,
                Y,
                Z,
                levels=20,
                colors='black',
                linewidths=0.5
            )

            ax.set_title(
                f"Time = {self.data[k]['time']:.4f}"
            )

            ax.set_xlabel("x")
            ax.set_ylabel("y")

            ax.set_xlim(0, 3)
            ax.set_ylim(0, 3)

            ax.set_aspect('equal')

            writer.grab_frame()

        writer.finish()
