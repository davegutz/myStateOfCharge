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
// Feb 7, 2021    DA Gutz        Created
// 
// Implement a general purpose 2-pole filter with variable update time.

clear
clear globals
mclose('all')
funcprot(0);
try close(figs); end
figs=[];
try
    xdel(winsid())
catch
end
exec('overplot.sce');


// Inline general purpose AB-2 Integrator with variable update time.
// Used when future prediction required to implement passive feedback without
// time skew
function [int_value, rstate, lstate, lim] = my_ab2_int_inline(in, T, ...
                            init_value, rstate, lstate, MIN, MAX, RESET)
    if RESET  then
        rstate = 0;
        lstate = init_value;
    else
        lstate = (3*in - rstate)*T/2 + lstate; // future
    end
    lim = %f;
    if lstate < MIN then
        lstate = MIN;
        lim = %t;
    else if lstate > MAX then
        lstate = MAX;
        lim = %t;
    end
    end
    rstate = in;
    int_value = lstate;
endfunction


// Inline general purpose Tustin Integrator with variable update time.
// Used when most accurate updative prediction required
function [int_value, rstate, lstate, lim] = my_tustin_int_inline(in, T, ...
                            init_value, rstate, lstate, MIN, MAX, RESET)
    if RESET  then
        rstate = 0;
        lstate = init_value;
    else
        lstate = (in + rstate)*T/2 + lstate; // present
    end
    lim = %f;
    if lstate < MIN then
        lstate = MIN;
        lim = %t;
    else if lstate > MAX then
        lstate = MAX;
        lim = %t;
    end
    end
    rstate = in;
    int_value = lstate;
endfunction


// Inline general purpose 2-pole filter with variable update time.
// Has advantage of have zeta > 1
function [lag_value, vel, accel, bstate, wstate, ab2_rstate, t_rstate] = ...
        my_2nd_order_inline(in, wn, zeta, T, bstate, wstate, ab2_rstate, t_rstate, ...
            MIN, MAX, RESET)
    a = 2*zeta*wn;
    b = wn*wn;
    if RESET then
        bstate = 0;
        vel = 0;
        accel = 0;
    else
        accel = b*(in - wstate) - a*bstate;
    end
   [vel, ab2_rstate, bstate, ab2_lim] = my_ab2_int_inline(accel, T, ...
                        accel, ab2_rstate, bstate, -%inf, %inf, RESET);  // future
   [lag_value, t_rstate, wstate, t_lim] = my_tustin_int_inline(vel, T, ...
                        vel, t_rstate, wstate, MIN, MAX, RESET);  // updative
   if t_lim then
       accel = 0;
       vel = 0;
   end
endfunction



T = 0.5;
wn = .05;  // r/s
zeta = 0.707;
MIN = -300;
MAX = 300;
N = 1000;
lag_value = zeros(N,1);
vel = zeros(N,1);
acc = zeros(N,1);
bstate = zeros(N,1);
wstate = zeros(N,1);
ab2_rstate = zeros(N,1);
t_rstate = zeros(N,1);
in = zeros(N,1);
time = zeros(N,1);
for j = 1:N,
    time(j) = (j-1)*T;
    if time(j) <1 then in(j) = 0;
        else if time(j)<225 then in(j) = in(j-1)+1;
            else if time(j) < 300 then in(j) = 450;
                else in(j) = 200;
            end
        end
    end
    if j==1, then RESET = %t; else RESET = %f; end
    if RESET then
        [lag_value(j), vel(j), acc(j), bstate(j), wstate(j), ab2_rstate(j), ...
            t_rstate(j)] = my_2nd_order_inline(in(j), wn, zeta, T, ...
            0, 0, 0, 0, MIN, MAX, RESET);
    else
        [lag_value(j), vel(j), acc(j), bstate(j), wstate(j), ab2_rstate(j), ...
            t_rstate(j)] = my_2nd_order_inline(in(j), wn, zeta, T, ...
            bstate(j-1), wstate(j-1), ab2_rstate(j-1), t_rstate(j-1), MIN, MAX, RESET);
    end
end
P.in = struct('time', time, 'values', in);
P.lag_value = struct('time', time, 'values', lag_value);
P.acc = struct('time', time, 'values', acc);
P.vel = struct('time', time, 'values', vel);
P.wstate = struct('time', time, 'values', wstate);
P.bstate = struct('time', time, 'values', bstate);

%size = [610, 460];
if ~exists('%zoom') then
    %zoom = '';
end
if ~exists('%loc') then
    %loc = default_loc + [50, 50];
end

figs($+1) = figure("Figure_name", '2nd Order Lag', "Position", [%loc, %size]);
subplot(2,1,1)
overplot(['P.acc', 'P.vel'], ['r-', 'b-', 'g-'], '2nd Order', 'time, s', %zoom)
subplot(2,1,2)
overplot(['P.in', 'P.lag_value'], ['b-', 'g-'], '2nd Order', 'time, s', %zoom)
    
