# debug why no error messages

import numpy as np
from numpy.random import randn
import Battery
from Battery import Battery, BatteryMonitor, BatterySim, is_sat, Retained
from Battery import overall_batt
from TFDelay import TFDelay
from MonSimNomConfig import *  # Global config parameters.   Overwrite in your own calls for studies
from datetime import datetime, timedelta
from Scale import Scale

if __name__ == '__main__':
    # from DataOverModel import SavedData, SavedDataSim, write_clean_file, overall
    # from DataOverModel import SavedData
    from DataOverModel import overall

    def main():
        y = np.sqrt(-1)

main()
