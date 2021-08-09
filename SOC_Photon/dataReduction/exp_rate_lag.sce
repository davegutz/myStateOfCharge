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


// Inline Exponential rate-lag
function [rate, lstate, rstate] = my_exp_rate_lag_inline(in, tau, T, ...
                                     rstate, lstate, MIN, MAX, RESET)
    eTt = exp(-T/tau);
    a = (tau/T) - eTt/(1-eTt);
    b = (1/(1-eTt)) - (tau/T);
    c = (1-eTt)/T;
    if RESET then
        rstate = in;
        lstate = in;
        rate = 0;
    else
        rate = c*( a*rstate + b*in - lstate);
        rate = min(max(rate, MIN), MAX);
        rstate = in;
        lstate = rate*T + lstate;
    end
endfunction

mdrate = zeros(100,1);
lstate = zeros(100,1);
rstate = zeros(100,1);
mdot = zeros(100,1);
dt = 0.01;
time = zeros(100,1);
for j = 1:100,
    time(j) = (j-1)*dt;
    if time(j) <0.05 then mdot(j) = .9; else  mdot(j) = mdot(j-1)+.01; end
    if j==1, then reset = %t; else reset = %f; end
    if reset then,
        [mdrate(j), lstate(j), rstate(j)] = my_exp_rate_lag_inline(mdot(j), 0.1, dt, ...
                                     0, 0, -%inf, %inf, reset)
    else
        [mdrate(j), lstate(j), rstate(j)] = my_exp_rate_lag_inline(mdot(j), 0.1, dt, ...
                                     rstate(j-1), lstate(j-1), -10, 10, reset)
    end
end
figure; plot(time, [mdot mdrate lstate, rstate]);
    
