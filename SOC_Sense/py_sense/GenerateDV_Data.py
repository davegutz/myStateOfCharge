# GenerateDV_Data:  Simulate v1 on a long term charge discharge cycle
# Copyright (C) 2023 Dave Gutz
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

"""Simulate a v1 verbose run on battery monitor"""

from datetime import datetime as dt
from myFilters import LagExp

build = 'g20230530_soc0p_ch'
chm = 1
qcrs = 360000.  # Coulombs full charge
sat = 1
sel = 1
mod = 0
bmso = 0
Tb = 25.
tau = 90.  # seconds hyst time constant (measured 1540. s)
time_start = 1691689394  # time of start simulation
T = 5.  # seconds update time
ctime = 0.  # seconds since boot
days_data = 4  # days of data collection
time = time_start
time_end = time_start + float(days_data) * 24. * 3600.
n = int((float(days_data) * 24. * 3600.)/ T) + 1
ib = 0.
soc = float(1.)
vb = 13.3
vsat = 13.7
v_sat = 14.6
v_normal = 13.3
dv_max = 0.2
dv_min = -0.06
dv_ds = 0.2  # change in voltage for soc
dv_di = 0.02  # change in voltage for current
voc = vb
voc_stat = vb
dv = dv_max
Hyst = LagExp(dt=T, max_=dv_max, min_=dv_min, tau=tau)
q = qcrs  # Coulombs charge
reset = True
csv_file = "generateDV_Data.csv"

with open(csv_file, "w") as output:
    for i in range(n):
        ctime = float(i) * T
        time = time_start + int(ctime)
        DATE = dt.utcfromtimestamp(time)
        time_str = DATE.strftime('%Y-%m-%d:%H:%M:%S')
        tod = DATE.hour
        ib_charge = 0.
        ib_hys = 0.
        if 8. < tod < 16.:
            ib_charge = 8.
        elif tod > 21. or tod < 6.:
            ib_charge = -5.
        if soc >= 1. and ib_charge > 0.:
            ib_charge = 0.
        ib_hys = ib_charge / 90.
        # if sat:
        #     ib_hys = dv_min
        ib = ib_charge
        q += ib_charge * T
        soc = max(min(q / qcrs, 1.), 0.)
        if soc > 0.98:
            voc_soc = v_sat + 0.02 - (1. - soc) * dv_ds
        else:
            voc_soc = v_normal - (1. - soc) * dv_ds
        dv = Hyst.calculate(ib_hys, reset, T)
        voc_stat = voc_soc
        dv_dyn = ib_charge * dv_di
        voc = voc_stat + dv
        vb = voc + dv_dyn
        ioc = float('nan')
        voc_ekf = float('nan')
        y_ekf = float('nan')
        soc_s = float('nan')
        soc_ekf = float('nan')
        soc_min = 0.
        Tbl = Tb
        sat = voc_soc > vsat
        if reset:
            output.write('unit,               hm,                  cTime,       dt,       chm,qcrs,sat,sel,mod,bmso, Tb,  vb,  ib,   ib_charge, ioc, voc_soc,    vsat,dv_dyn,voc_stat,voc_ekf,     y_ekf,    soc_s,soc_ekf,soc,soc_min, dv,\n')
        output.write("{:s}, {:s},{:12.3f},  {:6.3f},  {:2d},{:8.0f},{:2d}, {:2d}, {:2d}, {:2d},{:7.3f},{:6.3f},  {:7.3f}, {:10.3f},{:10.3f},{:7.3f},{:7.3f},{:7.3f},{:7.3f},{:7.3f},{:7.3f},{:7.3f},{:7.3f},{:7.3f},{:7.3f},{:7.3f},\n"\
              .format(build, time_str, ctime, 0.1, chm, qcrs, sat, sel, mod, bmso,
                      Tb, vb, ib, ib_charge, ioc, voc_soc, vsat, dv_dyn, voc_stat,
                      voc_ekf, y_ekf, soc_s, soc_ekf, soc, soc_min, dv))
        reset = False
