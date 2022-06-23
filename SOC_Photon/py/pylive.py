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

import numpy as np
from numpy.random import randn
import matplotlib.pyplot as plt

# use ggplot style for more sophisticated visuals
plt.style.use('ggplot')

def live_plotter(x_vec, y_vec, line1, line2, identifier='', pause_time=0.1):
    if line1 == []:
        # this is the call to matplotlib that allows dynamic plotting
        plt.ion()
        fig = plt.figure(figsize=(13, 6))
        ax = fig.add_subplot(111)

        # create a variable for the line so we can later update it
        line1,line2 = ax.plot(x_vec, y_vec, '-o', alpha=0.8)

        # update plot label/title
        plt.ylabel('Y Label')
        plt.title('Title: {}'.format(identifier))
        plt.show()

    # after the figure, axis, and line are created, we only need to update the y-data
    line1.set_ydata(y_vec[:,0])
    line2.set_ydata(y_vec[:,1])
    # line1.set_data(x_vec, y1_data)

    # adjust limits if new data goes beyond bounds
    if np.min(y_vec) <= line1.axes.get_ylim()[0] or np.max(y_vec) >= line1.axes.get_ylim()[1]:
        plt.ylim([np.min(y_vec) - np.std(y_vec), np.max(y_vec) + np.std(y_vec)])

    # this pauses the data so the figure/axis can catch up - the amount of pause can be altered above
    plt.pause(pause_time)

    # return line so we can update it again in the next iteration
    return line1, line2

def scale_n(y, lines, ax):
    # In multiple lined plot, all lines have same scale. Use lines[0]
    min_y = np.min(y)
    max_y = np.max(y)
    std_y = np.std(y)
    if min_y <= lines[0].axes.get_ylim()[0] or max_y >= lines[0].axes.get_ylim()[1]:
        ax.set_ylim([min_y-std_y, max(+std_y)])
    return ax

# if np.min(y_vec1[:][0:1]) <= linen1[0].axes.get_ylim()[0] or np.max(y_vec1[:][0:1]) >= linen1[0].axes.get_ylim()[1]:
#     ax1.set_ylim([ np.min(y_vec1[:][0:1]) - np.std(y_vec1[:][0:1]),   np.max(y_vec1[:][0:1]) + np.std(y_vec1[:][0:1]) ])


# if __name__ == '__main__':
    #from pylive import live_plotter
    # import numpy as np
    #
    # def main():
    #     size = 100
    #     x_vec = np.linspace(0, 1, size + 1)[0:-1]
    #     y_vec = np.random.randn(len(x_vec), 2)
    #     line1 = []
    #     line2 = []
    #     count = 0
    #     t = x_vec[-1]
    #     while True and count<30:
    #         count += 1
    #         y_vec[-1][0] = np.random.randn(1)
    #         y_vec[-1][1] = np.random.randn(1)
    #         line1,line2 = live_plotter(x_vec, y_vec, line1, line2)
    #         y_vec = np.append(y_vec[1:][:], np.zeros((1,2)), axis=0)
    #
    # main()
    
def liven_plotter(x, y, linen, fig, ax, subplot, ylabel, title, identifier='', pause_time=0.1):
    if linen is None:
        # this is the call to matplotlib that allows dynamic plotting
        plt.ion()
        ax = fig.add_subplot(subplot)

        # create a variable for the line so we can later update it
        linen = ax.plot(x, y, '-o', alpha=0.8)

        # update plot label/title
        plt.ylabel(ylabel)
        plt.title(title)
        ax = fig.add_subplot(subplot)
        plt.show()

    # after the figure, axis, and line are created, we only need to update the y-data
    linen[0].set_ydata(y[:, 0])
    linen[1].set_ydata(y[:, 1])

    # adjust limits if new data goes beyond bounds
    ax = scale_n(y, linen, ax)

    # this pauses the data so the figure/axis can catch up - the amount of pause can be altered above
    plt.pause(pause_time)

    return linen, ax


import numpy as np

size = 100
x_vec = np.linspace(0, 1, size + 1)[0:-1]
y_vec1 = np.random.randn(len(x_vec), 2)
y_vec2 = np.random.randn(len(x_vec), 2)*10.
linen1 = None
linen2 = None
count = 0
pause_time = 0.1
identifier=''
t = x_vec[-1]
count += 1
y_vec1[-1][0] = np.random.randn(1)
y_vec1[-1][1] = np.random.randn(1)
y_vec2[-1][0] = np.random.randn(1)*10.
y_vec2[-1][1] = np.random.randn(1)*10.
if linen1 is None:
    # this is the call to matplotlib that allows dynamic plotting
    plt.ion()
    fig = plt.figure(figsize=(13, 6))
    ax1 = fig.add_subplot(211)

    # create a variable for the line so we can later update it
    linen1 = ax1.plot(x_vec, y_vec1, '-o', alpha=0.8)

    # update plot label/title
    plt.ylabel('Y Label1')
    plt.title('Title: {}'.format(identifier))

    ax2 = fig.add_subplot(212)

    # create a variable for the line so we can later update it
    linen2 = ax2.plot(x_vec, y_vec2, '-o', alpha=0.8)

    # update plot label/title
    plt.ylabel('Y Label 2')
    plt.show()

# after the figure, axis, and line are created, we only need to update the y-data
linen1[0].set_ydata(y_vec1[:,0])
linen1[1].set_ydata(y_vec1[:,1])
linen2[0].set_ydata(y_vec2[:,0])
linen2[1].set_ydata(y_vec2[:,1])
# line1.set_data(x_vec, y1_data)

# adjust limits if new data goes beyond bounds
ax1 = scale_n(y_vec1, linen1, ax1)
ax2 = scale_n(y_vec2, linen2, ax2)

# this pauses the data so the figure/axis can catch up - the amount of pause can be altered above
plt.pause(pause_time)
y_vec1 = np.append(y_vec1[1:][:], np.zeros((1,2)), axis=0)
y_vec2 = np.append(y_vec2[1:][:], np.zeros((1,2)), axis=0)
