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
// Plotting
default_loc = [40 30];


function plot_data(%zoom, %loc)
    global figs
    %size = [610, 460];
    if ~exists('%zoom') then
        %zoom = '';
    end
    if ~exists('%loc') then
        %loc = default_loc + [50, 50];
    end
    figs($+1) = figure("Figure_name", 'Data', "Position", [%loc, %size]);
    subplot(211)
    overplot(['P.D.Tp_Sense', 'P.D.set', 'P.D.Ta_Sense', 'P.D.Ta_Filt', 'P.D.OAT'], ['r-', 'g-', 'm-', 'k--', 'b-'], 'Data temperatures', 'time, s', %zoom)
    subplot(212)
    overplot(['P.D.prop', 'P.D.integ', 'P.D.cont', 'P.D.pcnt_pot', 'P.D.cmd', 'P.D.duty'], ['r--', 'g--', 'b-', 'b--', 'c-', 'm--'], 'Data cmd', 'time, s', %zoom)
endfunction


function plot_compare(%zoom, %loc)
    global figs
    %size = [610, 460];
    if ~exists('%zoom') then
        %zoom = '';
    end
    if ~exists('%loc') then
        %loc = default_loc + [100, 100];
    end

    figs($+1) = figure("Figure_name", 'Data v Model -'+data_file, "Position", [%loc, %size]);
    subplot(211)
    overplot(['P.D.Tp_Sense', 'P.D.set', 'P.D.Ta_Sense', 'P.D.Ta_Filt', 'P.D.cmd_scaled', 'P.D.Ta_Obs'], ['r-', 'g-', 'm-', 'k--', 'b-', 'c--'], 'Data temp compare', 'time, s', %zoom)
    subplot(212)
    overplot(['P.D.solar_heat', 'P.D.heat_o', 'P.D.qduct', 'P.D.mdot', 'P.D.mdot_lag'], ['r-', 'b-', 'm-', 'k-', 'k--'], 'Data heat', 'time, s', %zoom)


    figs($+1) = figure("Figure_name", 'Tracker -'+data_file, "Position", [%loc, %size]);
    subplot(211)
    overplot(['P.D.Tp_Sense', 'P.D.Ta_Sense', 'P.D.duty_scaled'], ['r-', 'k-', 'g-'], 'Tp', 'time, s', %zoom)
    subplot(212)
    overplot(['P.D.solar_heat', 'P.D.heat_o', 'P.D.qduct'], ['r-', 'b-', 'm-'], 'Data heat', 'time, s', %zoom)

endfunction


function plot_all(%zoom, %loc)
    global figs
    if ~exists('%zoom') then
        %zoom = '';
        %loc = [40, 30];
        else if ~exists('%loc') then
            %loc = default_loc;
        end
    end

    P = map_all_plots(P, D, 'D');
    P = map_all_plots(P, B, 'B');

    plot_data(%zoom, %loc + [30 30]);
    plot_compare(%zoom, %loc + [60 60]);
endfunction


function zoom(%zoom, %loc)
    global figs
    if ~exists('%zoom') then
        %zoom = '';
        %loc = default_loc + [650, 0];
        else if ~exists('%loc') then
            %loc = default_loc + [650, 0];
        end
    end
    plot_all(%zoom, %loc);
endfunction


function plot_heat(%zoom, %loc)
    global figs
    %size = [610, 460];
    if ~exists('%zoom') then
        %zoom = '';
    end
    if ~exists('%loc') then
        %loc = default_loc + [50, 50];
    end
    figs($+1) = figure("Figure_name", 'Heat Data', "Position", [%loc, %size]);
    subplot(221)
    overplot(['P.C.cmd', 'P.C.cfm'], ['k-', 'b-'], 'Flow', 'time, s', %zoom)
    subplot(222)
    overplot(['P.C.Tp', 'P.C.Tml', 'P.C.Tms', 'P.C.Tdso', 'P.C.Ta', 'P.C.Tw', 'P.C.OAT'], ['k-', 'g--', 'm--', 'k-', 'c-', 'k-', 'g-'], 'Data cmd', 'time, s', %zoom)
    subplot(223)
    overplot(['P.C.Qdli', 'P.C.Qdsi', 'P.C.Qwi', 'P.C.Qwo', 'P.C.QaiMQao'], ['k-', 'b-', 'r-', 'g--', 'k--'], 'Flux', 'time, s', %zoom)
    subplot(224)
    overplot(['P.C.Ta', 'P.C.Tw'], ['b-', 'k-'], 'Duct', 'time, s', %zoom)

    figs($+1) = figure("Figure_name", 'Duct', "Position", [%loc, %size]);
    subplot(221)
    overplot(['P.C.cfm'], ['b-'], 'Flow', 'time, s', %zoom)
    subplot(222)
    overplot(['P.C.Tdso'], ['k-'], 'Tdso', 'time, s', %zoom)
    subplot(223)
    overplot(['P.C.Qdli', 'P.C.Qdlo'], ['k-', 'r-'], 'Flux', 'time, s', %zoom)
    subplot(224)
    overplot(['P.C.Qdsi', 'P.C.Qdso'], ['k-', 'r-'], 'Flux', 'time, s', %zoom)

endfunction


function plot_model(%zoom, %loc)
    global figs P
    %size = [610, 460];
    if ~exists('%zoom') then
        %zoom = '';
    end
    if ~exists('%loc') then
        %loc = default_loc + [50, 50];
    end

    figs($+1) = figure("Figure_name", 'Plant Poles', "Position", [%loc, %size]);
    subplot(111)
    overplot(['P.M.duct_pole', 'P.M.fast_room_pole', 'P.M.slow_room_pole'], ['k-', 'g-', 'r--'], 'Poles', 'time, s', %zoom)

    figs($+1) = figure("Figure_name", 'Heat Model', "Position", [%loc, %size]);
    subplot(221)
    overplot(['P.M.cmd', 'P.M.mdot', 'P.B.duty'], ['k-', 'b-', 'g-'], 'Flow', 'time, s', %zoom)
    subplot(222)
    overplot(['P.M.mdrate'], ['r-'], 'Flow', 'time, s', %zoom)

    figs($+1) = figure("Figure_name", 'Heat Model', "Position", [%loc, %size]);
    subplot(221)
    overplot(['P.M.cmd', 'P.M.cfm', 'P.B.duty', 'P.C.duty'], ['k-', 'b-', 'r-', 'g-'], 'Flow', 'time, s', %zoom)
    subplot(222)
    overplot(['P.M.Tp', 'P.M.Ta', 'P.M.Tw', 'P.M.OAT'], ['r-', 'k-', 'g-', 'b-'], 'Temps', 'time, s', %zoom)
    subplot(223)
    overplot(['P.M.Qai', 'P.M.Qao', 'P.M.Qwi', 'P.M.Qwo', 'P.M.Qconv'], ['r-', 'g-', 'r-', 'g-', 'b-'], 'Flux', 'time, s', %zoom)
    subplot(224)
    overplot(['P.M.Ta', 'P.B.Ta_Sense', 'P.M.Tw'], ['k-', 'c--', 'g-'], 'Duct', 'time, s', %zoom)

    figs($+1) = figure("Figure_name", 'Control', "Position", [%loc, %size]);
    subplot(221)
    overplot(['P.C.duty', 'P.B.duty', 'P.B.cmd'], ['g-', 'r-',  'b-'], 'Flow', 'time, s', %zoom)
    subplot(222)
    overplot(['P.M.Ta', 'P.B.Ta_Sense', 'P.B.Ta_Filt', 'P.B.set', 'P.B.Tp_Sense'], ['r-', 'm-', 'k--', 'g-', 'r--'], 'Flow', 'time, s', %zoom)
    subplot(223)
    overplot(['P.B.prop', 'P.B.integ', 'P.B.duty'], ['r-', 'b-', 'g--'], 'Flow', 'time, s', %zoom)
//    overplot(['P.B.prop', 'P.B.integ'], ['r-', 'b-', 'g-'], 'Flow', 'time, s', %zoom)
    subplot(224)
    overplot(['P.C.prop', 'P.C.integ', 'P.C.duty'], ['r-', 'b-', 'g--'], 'Flow', 'time, s', %zoom)

    figs($+1) = figure("Figure_name", 'Heat Model', "Position", [%loc, %size]);
    subplot(221)
    overplot(['P.M.TaDot', 'P.M.TwDot'], ['k-', 'g-'], 'Flow', 'time, s', %zoom)
    subplot(224)
    overplot(['P.M.Ta', 'P.B.Ta_Sense', 'P.B.Ta_Filt', 'P.M.Tw', 'P.M.Tass', 'P.M.Twss', 'P.M.Tkit'], ['k-', 'c--', 'g-', 'k--', 'r--', 'b--', 'c--'], 'Duct', 'time, s', %zoom)
    subplot(223)
    overplot(['P.M.dQa', 'P.M.Qmatch', 'P.M.Qconv', 'P.M.Qleak', 'P.M.QleakD', 'P.B.solar_heat'], ['r-', 'm--', 'g-', 'k-', 'b-', 'r--'], 'Flux', 'time, s', %zoom)
    subplot(222)
    overplot(['P.D.Tp_Sense'], ['r-'], 'Tp', 'time, s', %zoom)

endfunction

function plot_all_heat(%zoom, %loc)
    global figs
    if ~exists('%zoom') then
        %zoom = '';
        %loc = [40, 30];
        else if ~exists('%loc') then
            %loc = default_loc;
        end
    end
    plot_heat(%zoom, %loc + [30 30]);
endfunction

function plot_all_model(%zoom, %loc)
    global figs P
    if ~exists('%zoom') then
        %zoom = '';
        %loc = [40, 30];
        else if ~exists('%loc') then
            %loc = default_loc;
        end
    end


    plot_model(%zoom, %loc + [30 30]);
endfunction

function zoom_model(%zoom, %loc)
    global figs 
    if ~exists('%zoom') then
        %zoom = '';
        %loc = default_loc + [650, 0];
        else if ~exists('%loc') then
            %loc = default_loc + [650, 0];
        end
    end
    plot_all_model(%zoom, %loc);
endfunction
