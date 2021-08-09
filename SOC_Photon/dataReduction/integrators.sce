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
// Variable update time integrators.

exec('overplot.sce');


// Inline general purpose AB-2 Integrator with variable update time.
// Used when future prediction required to implement passive feedback without
// time skew
function [int_value, rstate, lstate] = my_ab2_int_inline(in, T, ...
                            init_value, rstate, lstate, MIN, MAX, RESET)
    if RESET  then
        rstate = 0;
        lstate = init_value;
    else
        lstate = min(max(  (3*in - rstate)*T/2 + lstate, MIN), MAX); // future
    end
    rstate = in;
    int_value = lstate;
endfunction


// Inline general purpose Tustin Integrator with variable update time.
// Used when most accurate updative prediction required
function [int_value, rstate, lstate] = my_tustin_int_inline(in, T, ...
                            init_value, rstate, lstate, MIN, MAX, RESET)
    if RESET  then
        rstate = 0;
        lstate = init_value;
    else
        lstate = min(max(  (in + rstate)*T/2 + lstate, MIN), MAX); // present
    end
    rstate = in;
    int_value = lstate;
endfunction

T = 0.01;
MIN = -3.0;
MAX = 2.5;
ab2_int_value = zeros(100,1);
ab2_lstate = zeros(100,1);
ab2_rstate = zeros(100,1);
t_int_value = zeros(100,1);
t_lstate = zeros(100,1);
t_rstate = zeros(100,1);
in = zeros(100,1);
time = zeros(100,1);
for j = 1:100,
    time(j) = (j-1)*dt;
    if time(j) <0.1 then in(j) = 0;
        else if time(j)<.6 then in(j) = 10;
            else if time(j)<1 then in(j) = -10;
                else in(j) = 0;
            end
        end
    end
    if j==1, then
        RESET = %t;
        [ab2_int_value(j), ab2_rstate(j), ab2_lstate(j)] = my_ab2_int_inline(in(j), T, ...
                                -1, 0,0, MIN, MAX, RESET)
        [t_int_value(j), t_rstate(j), t_lstate(j)] = my_ab2_int_inline(in(j), T, ...
                                -1, 0, 0, MIN, MAX, RESET)
    else
        RESET = %f;
        [ab2_int_value(j), ab2_rstate(j), ab2_lstate(j)] = my_ab2_int_inline(in(j), T, ...
                                -1, ab2_rstate(j-1), ab2_lstate(j-1), MIN, MAX, RESET)
        [t_int_value(j), t_rstate(j), t_lstate(j)] = my_ab2_int_inline(in(j), T, ...
                                -1, t_rstate(j-1), t_lstate(j-1), MIN, MAX, RESET)
    end
end
P.in = struct('time', time, 'values', in);
P.ab2_int_value = struct('time', time, 'values', ab2_int_value);
P.t_int_value = struct('time', time, 'values', t_int_value);

%size = [610, 460];
if ~exists('%zoom') then
    %zoom = '';
end
if ~exists('%loc') then
    %loc = default_loc + [50, 50];
end

figs($+1) = figure("Figure_name", 'Integrators', "Position", [%loc, %size]);
overplot(['P.ab2_int_value', 'P.t_int_value'], ['r-',  'g--'], 'AB-2 and Tustin', 'time, s', %zoom)
    
