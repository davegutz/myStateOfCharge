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
// Implement an exponential first order lag with variable update time.

exec('overplot.sce');


// Inline Exponential rate-lag wiht variable update time.
// Has advantage of remaining stable with tau=0.  Vary handy general
// purpose lag.
function [lag_value, lstate, rstate] = my_exp_lag_inline(in, tau, T, ...
                                     rstate, lstate, MIN, MAX, RESET)
    eTt = exp(-T/tau);
    meTt = 1-eTt;
    a = (tau/T) - eTt/meTt;
    b = (1/meTt) - (tau/T);
    c = meTt/T;
    if RESET then
        rstate = in;
        lstate = in;
        rate = 0;
    else
        rate = c*( a*rstate + b*in - lstate);
        rstate = in;
        lstate = min(max(rate*T + lstate, MIN), MAX);
    end
    lag_value = lstate;
endfunction

mdlag = zeros(100,1);
lstate = zeros(100,1);
rstate = zeros(100,1);
mdot = zeros(100,1);
dt = 0.01;
time = zeros(100,1);
for j = 1:100,
    time(j) = (j-1)*dt;
    if time(j) <0.05 then mdot(j) = .9; else if time(j)<.6 then mdot(j) = min(mdot(j-1)+.01, 1.3); else mdot(j) = 1.2; end; end
    if j==1, then reset = %t; else reset = %f; end
    if reset then,
        [mdlag(j), lstate(j), rstate(j)] = my_exp_lag_inline(mdot(j), 0.1, dt, ...
                                     0, 0, -%inf, %inf, reset)
    else
        [mdlag(j), lstate(j), rstate(j)] = my_exp_lag_inline(mdot(j), 0.1, dt, ...
                                     rstate(j-1), lstate(j-1), -1.25, 1.25, reset)
    end
end
P.mdot = struct('time', time, 'values', mdot);
P.mdlag = struct('time', time, 'values', mdlag);
P.lstate = struct('time', time, 'values', lstate);
P.rstate = struct('time', time, 'values', rstate);

%size = [610, 460];
if ~exists('%zoom') then
    %zoom = '';
end
if ~exists('%loc') then
    %loc = default_loc + [50, 50];
end

figs($+1) = figure("Figure_name", 'Exp Lag', "Position", [%loc, %size]);
overplot(['P.mdot', 'P.mdlag', 'P.lstate', 'P.rstate'], ['b-', 'r-',  'g--', 'c--'], 'Mdot', 'time, s', %zoom)
    
