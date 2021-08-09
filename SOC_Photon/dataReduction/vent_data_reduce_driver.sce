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

data_file = run_name + '.csv';
[data_t, data_d] = load_csv_data(data_file);
debug=2; plotting = %t; first_init_override = 1;

if size(data_d,1)==0 then
    error('debug.csv did not load.  Quitting\n')
end

[D, TD, first_init] = load_data(data_t, data_d);
if exists('first_init_override') then
    first_init = first_init_override;
else
    first_init_override = 0;
end
first = first_init;
last = D.N;
B = load_buffer(D, first, last);

//////////////////////////////////////////////////////////////////////////////////////
// Loop emulation

// Local logic initialization
//RESET = zeros(D.N, 1, 'boolean'); RESET([1]) = %t;
//hr_lgv = 0;
//C.hr_valid = D.spo2*0;

// Bandpass filter 1.5 - 4.5 Hz (0.11s - 0.03s)
//[C.ir_rate, C.ir_lstate, C.ir_rate_state] = my_exp_rate_lag(D.ir, 1/(LO_BAND*2*%pi), T, -RATE_LIM, RATE_LIM, RESET);
//[ir_filt_raw, C.ir_lag_state] = my_tustin_lag(C.ir_rate, 1/(HI_BAND*2*%pi), T, -5e4, 5e4, RESET);

// Read the first BUFFER_SIZE-SAMPLE_SIZE samples
//last = min(first+BUFFER_SIZE-SAMPLE_SIZE-1, D.N);
//B = load_buffer(D, first, last, %f);

global P
//P = map_all_plots(P, M, 'M');
P = map_all_plots(P, D, 'D');
P = map_all_plots(P, B, 'B');
//P.C.integ =  struct('time', B.time, 'values', C.integ);
//P.C.prop =  struct('time', B.time, 'values', C.prop);
//P.C.duty = struct('time', B.time, 'values', C.duty);

// Detail serial print
if debug>2 then serial_print_inputs_1(); end

// Plots
// Zoom last buffer
if plotting then
    zoom([-3600 D.time(last)])
    zoom([-900 D.time(last)])
    zoom([-120 D.time(last)])
    if debug>1 then
        plot_all()
    end
end

mclose('all');

//export_figs(figs, run_name);
