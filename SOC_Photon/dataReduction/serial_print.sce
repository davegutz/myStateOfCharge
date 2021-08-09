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

// Serial print functions


function serial_print_inputs(k)

    printf('%s,%18.3f,   %4.1f,%7.3f,%7.3f,', TD.hmString(k+1), D.time(k+1), D.set(k+1), D.Tp_Sense(k+1), D.Ta_Sense(k+1));

    mfprintf(doubtfd, '%s,%18.3f,   %4.1f,%7.3f,%7.3f,', TD.hmString(k+1), D.time(k+1), D.set(k+1), D.Tp_Sense(k+1), D.Ta_Sense(k+1));

endfunction


function serial_print(k)

    printf('%d,   %5.2f,%4.1f,%7.3f,  %7.3f,%7.3f,%7.3f,%7.3f,', D.duty(k+1), D.T(k+1), D.OAT(k+1), D.Ta_Obs(k+1), D.err(k+1), D.prop(k+1), D.integ(k+1), D.cont(k+1));

    mfprintf(doubtfd, '%d,   %5.2f,%4.1f,%7.3f,  %7.3f,%7.3f,%7.3f,%7.3f,', D.duty(k+1), D.T(k+1), D.OAT(k+1), D.Ta_Obs(k+1), D.err(k+1), D.prop(k+1), D.integ(k+1), D.cont(k+1));

endfunction


function serial_print_inputs_1()
    for k=0:size(D.set,1)-2
        serial_print_inputs(k);
        serial_print(k);
        printf('\n');
        mfprintf(doubtfd, '\n');
    end
end

function serial_print_model_1()
    for k=0:size(M.set,1)-2
        serial_print_inputs(k);
        serial_print(k);
        printf('\n');
        mfprintf(doubtfd, '\n');
    end
end

