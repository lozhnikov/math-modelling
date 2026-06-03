import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from abstract_plotter import AbstractPlotter


class ThreeBodyProblemPlotter(AbstractPlotter):
   
  def plot(self):
    steps = self.data.get("data", [])
    if not steps:
      raise ValueError("Нет данных для визуализации")

    trajectories = [[], [], []]

    for step in steps:
      bodies = step["data"]["bodies"]
      for i in range(3):
        pos = bodies[i]["position"]
        trajectories[i].append((pos["x"], pos["y"], pos["z"]))

    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')

    colors = ['red', 'green', 'blue']
    labels = ['Body 1', 'Body 2', 'Body 3']

    for i in range(3):
      trace = np.array(trajectories[i])
      ax.plot(trace[:, 0], trace[:, 1], trace[:, 2],
        color=colors[i], linewidth=1.5, label=labels[i])
      ax.scatter(trace[0, 0], trace[0, 1], trace[0, 2],
         color=colors[i], marker='o', s=50)
      ax.scatter(trace[-1, 0], trace[-1, 1], trace[-1, 2],
        color=colors[i], marker='s', s=50)

    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    ax.set_title('Траектории трёх тел под действием гравитации')
    ax.legend()
    plt.savefig(self.output_path, dpi=150)
    plt.close()