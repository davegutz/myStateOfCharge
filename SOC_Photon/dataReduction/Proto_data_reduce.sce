// Copyright (C) 2021 - Dave Gutz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// Feb 1, 2021    DA Gutz        Created
// 
// Plot and analyze ventilator data
// Collect data by
//   cygwin> python curlParticle.py
//   This script contains curl link command.
//   You may have to be logged into https://console.particle.io/devices
//   to have necessary permissions to curl
// The Particle files are in ../src
clear
clear globals
mclose('all')
funcprot(0);
exec('filt_functions.sce');
exec('load_data.sce');
exec('overplot.sce');
exec('serial_print.sce');
exec('plotting.sce');
exec('export_figs.sci');

global figs D C P
try close(figs); end
figs=[];
try
    xdel(winsid())
catch
end

// Local constants
exec('constants.sce');

// File to save results that look something like debug.csv
[doubtfd, err] = mopen('./doubtfProto.csv', 'wt');
if err then
    printf('********** close doubtf.csv ************\n');
end
mfprintf(doubtfd, 'doubtfProto.csv debug output of HR4C_data_reduce.sce\n');

// Load data.  Used to be done by hand-loading a sce file then >exec('debug.sce');
//run_name = 'vent_2021-01-31T19-25';
run_name = 'debugProto';
exec('vent_data_reduce_driver.sce');
