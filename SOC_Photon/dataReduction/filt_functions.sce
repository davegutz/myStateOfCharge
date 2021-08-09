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

// Filtering functions


// Sort x(size) in ascending order
// i, j 0-based, x is 1-based
function x = my_sort_ascend(x, %size) 
// Sort array in asclast order (insertion sort algorithm)
  i = int32(0);  j = int32(0);  n = double(0);
  for i = 1:%size-1
    n = x(i+1);
    j = i;
    while j>0 & n<x(j-1+1)
        x(j+1) = x(j);
        j = j-1;
    end
    x(j+1) = n;
  end
endfunction

    
// Vectorial Exponential rate-lag
function [rate, lstate, rstate] = my_exp_rate_lag(in, tau, T, ...
                                            MIN, MAX, RESET)
    N = size(in, 1);
    eTt = exp(-T/tau);
    a = (tau./T) - eTt./(1 - eTt);
    b = ((1)./(1 - eTt)) - (tau./T);
    c = (1 - eTt)./T;
    rate = zeros(N, 1);
    lstate = zeros(N, 1);
    rstate = zeros(N, 1);
    for i=2:N,
        if RESET(i-1) then
            lstate(i-1) = in(i-1);
            rstate(i-1) = in(i-1);
            rate(i-1) = 0;
        end
        rate(i) = c(i)*( a(i)*rstate(i-1) + b(i)*in(i) - lstate(i-1));
        rate(i) = min(max(rate(i), MIN), MAX);
        rstate(i) = in(i);
        lstate(i) = rate(i)*T(i) + lstate(i-1);
    end
endfunction

// Vectorial Tustin rate-lag
function [rate, state] = my_tustin_rate_lag(in, tau, T, ...
                            MIN, MAX, MIN_STATE, MAX_STATE, RESET)
    N = size(in, 1);
    a = (2*ones(N,1) ./ (2*tau + T));
    b = (2*tau - T) ./ (2*tau + T);
    rate = zeros(N, 1);
    for i=2:N,
        if RESET(i-1) then
            state(i-1) = min(max(in(i-1), MIN_STATE), MAX_STATE);
            rate(i-1) = 0;
        end
        rate(i) = a(i)*(in(i) - state(i-1));
        state(i) = in(i)*(1-b(i)) + state(i-1)*b(i);
        state(i) = min(max(state(i), MIN_STATE), MAX_STATE);
        rate(i) = min(max(rate(i), MIN), MAX);
    end
endfunction

// Vectorial Tustin lag
function [state, state] = my_tustin_lag(in, tau, T, ...
                                                MIN, MAX, RESET)
    N = size(in, 1);
    a = (2*ones(N,1) ./ (2*tau + T));
    b = (2*tau - T) ./ (2*tau + T);
    for i=2:N,
        if RESET(i-1) then
            state(i-1) = in(i-1);
        end
        state(i) = in(i)*(1-b(i)) + state(i-1)*b(i);
        state(i) = min(max(state(i), MIN), MAX);
    end
endfunction

// Inline Tustin lag
function [filt, state] = my_tustin_lag_inline(in, tau, T, state, ...
                                                MIN, MAX, RESET)
    a =  2 / (2*tau + T);
    b = (2*tau - T) / (2*tau + T);
    if RESET then
        state = in;
    else
        state = in*(1-b) + state*b;
        state = min(max(state, MIN), MAX);
    end
    filt = state;
endfunction


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

// Deadband
function y = dead(x, hdb)
    // x    input
    // hdb  half of deadband
    // y    output
    y = max(x-hdb, 0) + min(x+hdb, 0);
endfunction

