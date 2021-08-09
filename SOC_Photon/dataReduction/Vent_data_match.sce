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
mclose('all')
funcprot(0);
clear figs D C P M
clear globals
exec('filt_functions.sce');
exec('load_data.sce');
exec('overplot.sce');
exec('serial_print.sce');
exec('plotting.sce');
exec('export_figs.sci');
exec('heat_model.sce');


global figs
try close(figs); end
figs=[];
try
    xdel(winsid())
catch
end

// Local constants
exec('constants.sce');

// File to save results that look something like debug.csv
[doubtfd, err] = mopen('./doubtf.csv', 'wt');
if err then
    printf('********** close doubtf.csv ************\n');
end
mfprintf(doubtfd, 'doubtf.csv debug output of HR4C_data_reduce.sce\n');

// Initialize model
M="";C="";
exec('heat_model_constants.sce');
[M, C] = heat_model_define();

/////////////////////////////////  begin user input  ///////////////////////////////////////////////////////
// User inputs
force_init_ta = %f; debug=2; plotting = %t; first_init_override = 1;
closed_loop = %f; do_poles = %f;

// Transient runs to choose from

// Fan off;   open door at -17642;  Fan comes on at -11040
//run_name = 'vent_2021-02-22T03-46_open'; M.dTpTk = 80; M.t_door_open = -17642; M.Qlk = 669-15.4*M.Glk;  M.Qlkd = 900-42.73*M.Glkd;

// Normal cooldown then normal shutoff for night.  Clock timer on/off not included in scilab model (sys_on/sys_off)
//run_name = 'vent_2021-02-23T19-00_';M.t_sys_off = -20700;M.t_sys_on = -2720; M.mdotl_decr = 6000; M.Qlk = -100; M.Qlkd = 736-42.73*M.Glkd;

// Fan on;  close door at -20741; crack door at -17130; open door  at -11000
// For this one, I think conduction partially blocked when door cracked.   Shouldn't use this one
//run_name = 'vent_2021-02-17T04-00_open_open_100'; M.t_door_close = -20741; M.t_door_crack =  -17130; M.t_door_open = -11000; M.Qlk = -480; M.Qlkd = -4000; M.t1 = 1200; M.t2 = 2000; M.Gconv = 120;


// Forcing various duty values through a cold day.   At 11 am OAT=19, Tw=65, Tk=70
run_name = 'vent_2021-03-02T04-31_'; M.Tk=70; M.Qlkd = -4000; 


// TODO:  test convection transitions various duty cycles
// TODO:  redo door cracked to open it a bit more but without engaging convection
// TODO:  rederive plant poles with new Gconv/Glk/Glkd

/////////////////////////////////  end user input  ///////////////////////////////////////////////////////

// Load data.  Used to be done by hand-loading a sce file then >exec('debug.sce');
data_file = run_name + '.csv';
[data_t, data_d] = load_csv_data(data_file);
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
B.N = size(B.time, 1);
B.Tf = zeros(D.N, 1);
[M, C] = heat_model_init(M, C);


// Main loop
time_past = B.time(1);
reset = %t;

for i=1:B.N,
    if i==1 then, reset = %t; end
    time = B.time(i);
    dt = time - time_past;
    time_past = time;
    // Inputs
    %cmd = B.cmd(i);
    if reset then, 
        duty = B.duty(1);
    else
        if closed_loop then
            duty = C.duty(i-1);
        else
            duty = B.duty(i);
        end
    end
    Tp = B.Tp_Sense(i);
    OAT = B.OAT(i);
    pcnt_pot = B.pcnt_pot(i);
    ta_init = B.Ta_Sense(i);
    [M, a, b, c, dMdot_dCmd] = total_model(time, dt, Tp, OAT, duty, reset, M, i, B, do_poles);

    // Linear  model stuff
    if do_poles then
        sroom = syslin('c', a,b,c);
        hroom = ss2tf(sroom);
        hduct_den = poly([-1/M.mdotl_incr], 's');  // mdotl_incr is worst case
        hduct_num = dMdot_dCmd/M.mdotl_incr;
        hduct = hduct_num / hduct_den;
        sysplant = hduct * hroom;
        Sduct = zpk(hduct);
        duct_poles = -gsort(1 ./Sduct.P{:});
        Sroom = zpk(hroom);
        room_poles = -gsort(1 ./ Sroom.P{:});
        Splant = zpk(sysplant);
        our_poles = -gsort(1 ./ Splant.P{:});
        M.duct_pole(i) = duct_poles(1);
        M.fast_room_pole(i) = room_poles(1);
        if size(room_poles, 1)<2 then
            M.slow_room_pole(i) = %nan;
        else
            M.slow_room_pole(i) = room_poles(2);
        end
        sysc_num = poly([-1/C.tau], 's')*C.tau*C.G;
        sysc_den = poly([0], 's');
        sysc = sysc_num/sysc_den;
        sysol = sysc * sysplant;
    end

    // Control law
    %set = B.set(i);
    err = %set - M.Ta(i);
    err_comp = dead(err, C.DB)*C.G;
    C.prop(i) = max(min(err_comp * C.tau, 20), -20);
    if reset then,
        C.integ(i) = duty;
    else
        C.integ(i) = max(min(C.integ(i-1) + dt*err_comp, pcnt_pot - C.prop(i)), -C.prop(i));
    end
    cont = max(min(C.integ(i)+C.prop(i), pcnt_pot), 0);
    %cmd = max(min(min(pcnt_pot, cont), 100), 0);
    C.duty(i) = %cmd;
    reset = %f;
end

global P
P = map_all_plots(P, M, 'M');
P = map_all_plots(P, D, 'D');
P = map_all_plots(P, B, 'B');
P.C.integ =  struct('time', B.time, 'values', C.integ);
P.C.prop =  struct('time', B.time, 'values', C.prop);
P.C.duty = struct('time', B.time, 'values', C.duty);

// Detail serial print
if debug>2 then serial_print_model_1(); end

// Plots
// Zoom last buffer
if plotting then
    if debug>1 then
        plot_all_model()
    end
//    zoom_model([-25000 0])
end

mclose('all')
//export_figs(figs, run_name)
