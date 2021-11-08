# Play with Kalman filters from Kalman-and-Bayesian-Filters-in-Python
# Press the green button in the gutter to run the script.
# install packages using 'python -m pip install numpy, matplotlib, scipy, pyfilter
# References:
#   [2] Roger Labbe. "Kalman and Bayesian Filters in Python"  -->kf_book
#   https://github.com/rlabbe/Kalman-and-Bayesian-Filters-in-Pythonimport numpy as np
# Dependencies:  python3.10,  ./pyDAGx


# Prepare for EKF with function call for battery
from pyDAGx.lookup_table import LookupTable
lut = LookupTable()
