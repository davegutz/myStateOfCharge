import numpy as np
from numpy.random import randn
import matplotlib.pyplot as plt
from numpy.random import normal
from kf_book.book_plots import set_figsize, figsize
from kf_book.nonlinear_plots import plot_nonlinear_func
import kf_book.nonlinear_internal as nonlinear_internal

import book_format
book_format.set_style()

data = normal(loc=0., scale=1., size=500000)
