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

// Load data
function [data_t, data_d] = load_csv_data(csv_file)
    // Load csv file of various columns but data of interest has fixed columns and key in first colum
    // Read all the value in one pass
    // then using csvTextScan is much more efficient
    text = mgetl(csv_file);
    textS = [text(:, 2:$) text(:, 1)];
    textS = gsort(text, 'r', 'i');
    text = unique(textS, "r");  // delete repeated data
    text = [text(:,$) text(:, 1:$-1)];
    clear n_commas
    for l = 1:size(text, 1)
        n_commas($+1) = size(strindex(text(l), ','), 2);
    end
    nc_data = median(n_commas);  // works for most cases
    i_data = find(n_commas==nc_data);  // all rows with nc_data length
    data_t = csvTextScan(text(i_data), ',', '.', 'string');
    data_d = csvTextScan(text(i_data), ',', '.', 'double');
    data_dS = [data_d(:, 2:$) data_d(:, 1)];  // move text date out of first column
    data_dS = gsort(data_dS, 'lr', 'i');        // sort by time number
    data_d = unique(data_dS, "r");   // delete repeated data (yes!  had to do it twice)
    data_d = [data_d(:,$) data_d(:,1:$-1)];  // restore the order
endfunction


function [D, TD, first] = load_data(data_t, data_d)
    D.N = size(data_d, 1);
    time_adj = data_d(D.N, 2);
    TD.hmString = data_t(:, 1);
    D.time = data_d(:, 2) - time_adj;
    D.set = data_d(:, 3);
    D.Tp_Sense = data_d(:, 4);
    D.Ta_Sense = data_d(:, 5);
    D.cmd = data_d(:, 6);
    D.T = data_d(:, 7);
    D.OAT = data_d(:, 8);
    D.Ta_Obs = data_d(:, 9);
    D.err = data_d(:, 10);
    D.prop = data_d(:, 11);
    D.integ = data_d(:, 12);
    D.cont = data_d(:, 13);
    D.pcnt_pot = data_d(:, 14);
    D.duty = data_d(:, 15)/2.55;
    D.Ta_Filt = data_d(:, 16);
    D.solar_heat = data_d(:, 17);
    D.heat_o = data_d(:, 18);
    D.qduct = data_d(:, 19);
    D.mdot = data_d(:, 20);
    D.mdot_lag = data_d(:, 21);
    D.cmd_scaled = D.cmd/100*20+60;
    D.duty_scaled = D.duty/400*20+60;
    first = 1;
endfunction


function B = load_buffer(D, first, last);
    B.time = D.time(first:last);
    B.set = D.set(first:last);
    B.Tp_Sense = D.Tp_Sense(first:last);
    B.Ta_Sense = D.Ta_Sense(first:last);
    B.cmd = D.cmd(first:last);
    B.T = D.T(first:last);
    B.OAT = D.OAT(first:last);
    B.Ta_Obs = D.Ta_Obs(first:last);
    B.err = D.err(first:last);
    B.prop = D.prop(first:last);
    B.integ = D.integ(first:last);
    B.cont = D.cont(first:last);
    B.pcnt_pot = D.pcnt_pot(first:last);
    B.duty = D.duty(first:last);
    B.cmd_scaled = D.cmd_scaled(first:last);
    B.Ta_Filt = D.Ta_Filt(first:last);
    B.solar_heat = D.solar_heat(first:last);
    B.heat_o = D.heat_o(first:last);
endfunction
