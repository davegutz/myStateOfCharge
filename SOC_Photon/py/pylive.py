# LivePlotExample:  develop real time potting
# Copyright (C) 2022 Dave Gutz
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation;
# version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# See http://www.fsf.org/licensing/licenses/lgpl.txt for full license text.

"""
from https://makersportal.com/blog/2018/8/14/real-time-graphing-in-python
In data visualization, real-time plotting can be a powerful tool to analyze data as it streams into the acquisition
system. Whether temperature data, audio data, stock market data, or even social media data - it is often advantageous
to monitor data in real-time to ensure that instrumentation and algorithms are functioning properly.

In this tutorial, I will outline a basic function written in Python that permits real-time plotting of data. The
function is simple and straight-forward, but its powerful result allows any researcher or data analyst to take full
advantage of data monitoring as it streams into the user's computer!

A few notes on the function:
  - line1.set_ydata(y1_data) can also be switched to line1.set_data(x_vec,y1_data) to change both x and y data on the plots.
  - plt.pause() is necessary to allow the plotter to catch up - I've been able to use a pause time of 0.01s without any issues
  - The user will need to return line1 to control the line as it is updated and sent back to the function
  - The user can also customize the function to allow dynamic changes of title, x-label, y-label, x-limits, etc.

Notice how in the script, I do not re-plot the x-axis data. This enables us to quickly update the y-data. This
is the fast-moving advantage of the line1.set_ydata(y1_data) method as opposed to the traditional plt.plot() method.
The script above could also be used to update both x and y data, but more issues arise when handling both x and y
movement. The x-axis limits would need to be actively moving its bounds, as well as the y-axis limits. This is not
impossible, however, I think one workaround advantage is to simply change the x-axis tick labels instead and leave the
actual limits alone. To actively update the x-axis tick labels use the following method:
line1.axes.set_xticklabels(date_vector)
This will maintain the limits, the x-axis tick alignments, the bounds, but change the labels on the x-axis.
"""

from numpy.random import randn
import matplotlib.pyplot as plt

# use ggplot style for more sophisticated visuals
# plt.style.use('ggplot')


def scale_n(y, lines, ax):
    # In multiple lined plot, all lines have same scale. Use lines[0]
    min_y = np.min(y)
    max_y = np.max(y)
    std_y = np.std(y)
    ylim_0 = lines[0].axes.get_ylim()[0]
    ylim_1 = lines[0].axes.get_ylim()[1]
    if min_y <= ylim_0 or max_y >= ylim_1:
        ax.set_ylim([min_y-std_y, max_y+std_y])
    return ax


def liven_plotter(x, y, linen, fig, labels=None, subplot=111, ax=None, y_label='', title='', pause_time=0.1,
                  symbol=''):
    if linen is None:
        # this is the call to matplotlib that allows dynamic plotting
        plt.ion()
        ax = fig.add_subplot(subplot)

        # create a variable for the line, so we can later update it
        if labels is None:
            linen = ax.plot(x, y, symbol, alpha=0.8)
        else:
            linen = ax.plot(x, y, symbol, alpha=0.8, label=labels)
            plt.legend(loc=2)
        # update plot label/title
        plt.ylabel(y_label)
        plt.title(title)
        plt.show()

    # after the figure, axis, and line are created, we only need to update the y-data
    m = np.shape(y)[1]
    for i in range(m):
        linen[i].set_ydata(y[:, i])

    # adjust limits if new data goes beyond bounds
    ax = scale_n(y, linen, ax)

    # this pauses the data so the figure/axis can catch up - the amount of pause can be altered above
    plt.pause(pause_time)

    return linen, ax


if __name__ == '__main__':
    import numpy as np

    def main():
        size = 100
        x_vec = np.linspace(0, 1, size + 1)[0:-1]
        y_vec1 = np.random.randn(len(x_vec), 2)
        y_vec2 = np.random.randn(len(x_vec), 1) * 10.
        linen_x1 = None
        linen_x2 = None
        axx1 = None
        axx2 = None
        fign = None
        count = 0
        identifier = ''
        count_max = 100
        while True and count < count_max:
            count += 1
            print(count, "of", count_max)
            y_vec1[-1][0] = np.random.randn(1)
            y_vec1[-1][1] = np.random.randn(1)
            y_vec2[-1][0] = np.random.randn(1) * 10.
            # y_vec2[-1][1] = np.random.randn(1) * 10.
            if linen_x1 is None:
                fign = plt.figure(figsize=(12, 5))
            linen_x1, axx1 = liven_plotter(x_vec, y_vec1, linen_x1, fign, subplot=211, ax=axx1,
                                           y_label='Y Label1', title='Title: {}'.format(identifier),
                                           pause_time=0.1, labels=['v11', 'v12'])
            linen_x2, axx2 = liven_plotter(x_vec, y_vec2, linen_x2, fign, subplot=212, ax=axx2,
                                           y_label='Y Label2', pause_time=0.1, labels=['v21'])
            y_vec1 = np.append(y_vec1[1:][:], np.zeros((1, 2)), axis=0)
            y_vec2 = np.append(y_vec2[1:][:], np.zeros((1, 1)), axis=0)

    main()
