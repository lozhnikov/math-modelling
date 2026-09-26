from matplotlib import pyplot as plt
import numpy as np
import matplotlib.animation as manimation
from mpl_toolkits.mplot3d import Axes3D
import json
from plotters.abstract_plotter import AbstractPlotter


class HeatConductionPlotter(AbstractPlotter):

    def plot(self):
      M = self.data[0]['data']['M']
      frames = len(self.data)

      N = 3 * M + 1

      fig = plt.figure(figsize=(10, 8))
      ax = fig.add_subplot(
        111,
        projection='3d'
      )

      writer = manimation.FFMpegWriter(fps=25)
      writer.setup(fig, self.output_path, 200)

      #manimation.save('animation.gif', writer='pillow', fps=30)

      x = np.linspace(0, 3, N)
      y = np.linspace(0, 3, N)
      X, Y = np.meshgrid(x, y)

      for t in range(frames):

        Z = np.array(
          self.data[t]['data']['fn'],
          dtype=float
        )

        Z = Z.reshape((N, N))

            # --------------------------------------------------
            # Внутренний квадрат:
            # 1 < x < 2
            # 1 < y < 2
            # --------------------------------------------------

        mask = (
          (X > 1.0) &
          (X < 2.0) &
          (Y > 1.0) &
          (Y < 2.0)
        )

        Z = np.ma.array(Z, mask=mask)

        ax.clear()

        # 3D-поверхность
        ax.plot_surface(
          X,
          Y,
          Z,
          cmap='jet',
          linewidth=0,
          antialiased=True
        )


        # Рисуем границу внутреннего квадрата
        ax.plot(
          [1, 2], [1, 1], [Z.min(), Z.min()],
          color='black', linewidth=2
        )

        ax.plot(
          [2, 2], [1, 2], [Z.min(), Z.min()],
          color='black', linewidth=2
        )

        ax.plot(
          [2, 1], [2, 2], [Z.min(), Z.min()],
          color='black', linewidth=2
        )

        ax.plot(
          [1, 1], [2, 1], [Z.min(), Z.min()],
          color='black', linewidth=2
        )



        ax.set_title(
          f"Heat equation, "
          f"Time = {self.data[t]['time']:.4f}"
        )

        ax.set_xlabel("x")
        ax.set_ylabel("y")
        ax.set_zlabel("u(x, y, t)")

        #границы координатной сетки
        ax.set_xlim(0, 3)
        ax.set_ylim(0, 3)
        ax.set_zlim(
          np.min(Z),
          np.max(Z)
        )

        ax.view_init(
          elev=45,
          azim=-120
        )

        writer.grab_frame()


      writer.finish()
