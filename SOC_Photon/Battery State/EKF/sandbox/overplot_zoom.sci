// Copyright (C) 2019 - Dave Gutz
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
// Jul 14, 2019    DA Gutz        Created
//
function overplot_zoom(sig_names, c, %title, %tlim, %ylim)
    if length(%tlim) then
        zooming = %t;
        xmin = %tlim(1); xmax = %tlim(2);
    else
        xmin = 0;
        xmax = evstr(sig_names(1)+'.time($)');
        zooming = %f;
    end
    ymin = %inf;  ymax = -%inf;
    if argn(2)>4 then
        if length(%ylim) then
            yooming = %t;
            ymin = %ylim(1); ymax = %ylim(2);
        else
            yooming = %f;
        end
    else
        yooming = %f;
    end
    xtitle(%title)
    [n, m] = size(sig_names);
    [nc, mc] = size(c);
    for i=1:m
        plot(evstr(sig_names(i)+'.time'), evstr(sig_names(i)+'.values'), c(i))
        if zooming then
            zoom_range = find(evstr(sig_names(1)+'.time')>%tlim(1) & ...
                    evstr(sig_names(1)+'.time')<%tlim(2));
            if ~yooming then
                ymin = min(ymin, min(evstr(sig_names(i)+'.values(zoom_range)')));
                ymax = max(ymax, max(evstr(sig_names(i)+'.values(zoom_range)')));
            end
        end
    end
    set(gca(),"grid",[1 1])
    legend(sig_names);
    if zooming | yooming then
        set(gca(),"data_bounds", [xmin xmax ymin ymax])
    end
endfunction


   figs($+1) = figure("Figure_name", Title, "Position",  [440,0,500,460]); clf();
    subplot(211)
    overplot_zoom(['Ctrl.A8_POS_REF', 'Ctrl.A8_POS', 'Ctrl.A8P_BIAS'], ['b-', 'g-', 'r-'], 'Stroke  '+Title, xlims)
    subplot(212)
    overplot_zoom(['Ctrl.A8_TM_DEM', 'R.A8_LEAK_FAULT', 'Ctrl.A8P_QUIET', 'Ctrl.A8P_CL_RECENT', 'Ctrl.A8DT_REF'], ['k-', 'b-', 'g-', 'r-', 'm-'], 'TM Current', xlims)

else

    figs($+1) = figure("Figure_name", 'A8 Servo  ' +Title, "Position",  [440,0,500,460]); clf();
    subplot(221)
    overplot_zoom(['Ctrl.A8_POS_REF', 'Ctrl.A8_POS', 'Ctrl.A8P_BIAS_LIM', 'Ctrl.A8P_BIAS'], ['b-', 'g-', 'r-', 'k--'], 'Stroke  '+Title, xlims)

    figs($+1) = figure("Figure_name", 'Lim Cycle  ' +Title, "Position",  [480,0,500,460]); clf();
    subplot(111)
    overplot_zoom(['Ctrl.A8_TM_DEM', 'Ctrl.A8_TM_DEM_LK', 'Ctrl.A8P_POS_DT', 'Ctrl.A8P_REF_DRIFT', 'Ctrl.A8P_QUIET', 'Ctrl.A8P_STEADY' ], ['r-', 'r--', 'b-', 'k-', 'r-', 'b--'], 'TM Current', xlims)

    if 0
    figs($+1) = figure("Figure_name", 'A8 Servo Init ' +Title, "Position",  [480,0,500,460]); clf();
    subplot(221)
    overplot_zoom(['Ctrl.A8_POS_REF', 'Ctrl.SI0411'], ['b-', 'g--'], 'Ref  '+Title, xlims)
    subplot(223)
    overplot_zoom(['Ctrl.A8_ERR', 'Ctrl.SI0402'], ['k-', 'c--'], 'TM Current', xlims)
    subplot(222)
    overplot_zoom(['Ctrl.A8_POS_REF', 'Ctrl.SI0411', 'Ctrl.SI0406'], ['b-', 'g--', 'm--'], 'Ref  '+Title, xlims)
    end

    figs($+1) = figure("Figure_name", 'Leak Fail  '+Title, "Position", [440,30,1000,660]); clf();
    subplot(321)
    overplot_zoom(['Ctrl.A8_TM_DEM', 'Ctrl.A8_TM_DEM_LK', 'Act.XMAVEN', 'Act.XMAVNC'], ['r-', 'r--', 'k-', 'm--'], 'Leak Error  ' +Title, xlims, [-25 60])
    subplot(322)
    overplot_zoom(['Ctrl.A8_A_POS_RATE', 'R.A8_LEAK_RAT_MAX', 'R.A8_LEAK_RAT_MIN'], ['b-', 'b--', 'b--'], 'Rate', xlims)
    subplot(323)
    overplot_zoom(['R.A8_LEAK_FAULT'], ['c-'], 'Leak Trip', xlims)
    subplot(324)
    ylims = [0 F.A8_LK_PERSIST];
    overplot_zoom(['R.A8_LEAK_TIM', 'R.A8_LEAK_TRIP_TIME_LIM'], ['r-', 'c--', 'r--'], 'Leak Trip Time', xlims, ylims)
    subplot(325)
    overplot_zoom(['Ctrl.A8_LEAK'], ['r--'], 'Leak Fail', xlims)
    subplot(326)
    overplot_zoom(['Ctrl.A8_TM_NULL_SHIFT'], ['r--'], 'Null Shift', xlims)

    figs($+1) = figure("Figure_name", 'Hydro Fail  '+Title, "Position", [440,60,1000,660]); clf();
    subplot(321)
    overplot_zoom(['Ctrl.A8_ERR', 'R.A8_HYDRO_ERR_MAX', 'R.A8_HYDRO_ERR_MIN', 'R.A8_HYDRO_ERRTRIP_HI', 'R.A8_HYDRO_ERRTRIP_LO'], ['r-', 'r--', 'r--', 'm-', 'c--'], 'Hydro Error  ' + Title, xlims)
    subplot(322)
    overplot_zoom(['Ctrl.A8_A_POS_RATE', 'R.A8_HYDRO_RAT_MAX', 'R.A8_HYDRO_RAT_MIN', 'R.A8_HYDRO_RAT_HI', 'R.A8_HYDRO_RAT_LO'], ['b-', 'b--', 'b--', 'm-', 'c--'], 'Rate', xlims)
    subplot(323)
    overplot_zoom(['R.A8_HYDRO_FAULT_HI', 'R.A8_HYDRO_FAULT_LO'], ['c-', 'k--'], 'Hydro Trip', xlims)
    subplot(324)
    ylims = [0 F.A8_LOOP_PERSIS];
    overplot_zoom(['R.A8_HYDRO_TIME_HI', 'R.A8_HYDRO_TIME_LO', 'R.A8_HYDRO_TRIP_TIME_LIM'], ['r-', 'c--', 'r--'], 'Hydro Trip Time', xlims, ylims)
    subplot(325)
    overplot_zoom(['Ctrl.A8_HYDRO_TRIP', 'Ctrl.A8_HYDRO_FAIL'], ['c-', 'r--'], 'Hydro Fail', xlims)


    figs($+1) = figure("Figure_name", 'Stuck  '+Title, "Position", [440,90,1000,660]); clf();
    subplot(321)
    overplot_zoom(['Ctrl.A8P_POS_DT', 'R.A8P_upper_quiet', 'R.A8P_lower_quiet'], ['r-', 'r--', 'r--'], 'Quiet  '+Title, xlims)
    subplot(322)
    overplot_zoom(['Ctrl.A8P_OP_STK', 'Ctrl.A8P_NUM_REQ', 'Ctrl.A8P_CL_STK', 'Ctrl.A8P_BIAS', 'Ctrl.LZ0423'], ['m-', 'b-', 'r-', 'g-', 'c-'], 'Sticking', xlims)
    subplot(323)
    overplot_zoom(['Ctrl.A8_POS_REF', 'Ctrl.A8_POS', 'Ctrl.A8P_POS_REF_FILT', 'Ctrl.A8P_POS_FILT_LOC'], ['r-', 'k-', 'r--', 'k--'], 'Results', xlims)
    subplot(324)
    overplot_zoom(['Ctrl.A8P_MOTION', 'Ctrl.A8P_REF_DRIFT', 'R.A8P_free_op_trip', 'R.A8P_free_cl_trip', 'R.A8P_Ref_drift_cl_trip', 'R.A8P_Ref_drift_op_trip'], ['r-', 'b-', 'r--', 'r--', 'b--', 'b--'], 'RunFree', xlims)
    subplot(325)
    ylims = [-10 10];
    overplot_zoom(['Ctrl.A8P_OP_STK_', 'Ctrl.A8P_CL_STK_', 'Ctrl.A8P_QUIET_', 'Ctrl.A8P_STEADY_', 'Ctrl.A8P_STK_WATCH', 'Ctrl.A8P_NUM_REQ_', 'Ctrl.A8P_NUM_', 'Ctrl.A8P_OP_ENABLE_', 'Ctrl.A8P_CL_ENABLE_'], ['r-', 'r--', 'b-', 'g--', 'm-', 'c--', 'c-', 'k--', 'k-'], 'Status', xlims, ylims)
    subplot(326)
    ylims = [-1 20];
    Ctrl.PLA_q10 = Ctrl.PLA; Ctrl.PLA_q10.values = Ctrl.PLA_q10.values/10;
    overplot_zoom(['Ctrl.A8P_OP_STK_SOON', 'Ctrl.A8P_CL_STK_SOON', 'Ctrl.AB_PERM', 'Ctrl.PLA_q10'], ['m-', 'r-', 'c--', 'k'], 'Status', xlims, ylims)

    figs($+1) = figure("Figure_name", 'Load  '+Title, "Position", [440,120,1000,660]); clf();
    subplot(321)
    overplot_zoom(['Act.VEN_LOAD', 'Ctrl.VEN_LOAD_X', 'Act.VEN_LOAD_CALC', 'Act.VEN_FRICTION'], ['k-', 'g--', 'r--', 'm-'], 'Load  '+Title, xlims)
    subplot(322)
    overplot_zoom(['Act.PDVEND', 'Act.PDVEN'], ['k--', 'r-'], 'Pressure', xlims)
    subplot(323)
    overplot_zoom(['Act.VEN_SVENO', 'Act.VEN_SVENC'], ['g-', 'r-'], 'Scalars', xlims)
    subplot(324)
    overplot_zoom(['Act.VEN_FMO', 'Act.VEN_FMC'], ['g-', 'r-'], 'Force Margin', xlims)
    subplot(325)
    overplot_zoom(['Act.VEN_FMO', 'Act.VEN_FMC'], ['g-', 'r-'], 'Force Margin', xlims)
    subplot(326)
    overplot_zoom(['Act.VEN_XDTD', 'Act.VEN_XDT'], ['g-', 'r-'], 'Rate', xlims)

    figs($+1) = figure("Figure_name", 'CPR  '+Title, "Position", [440,150,1000,660]); clf();
    subplot(321)
    overplot_zoom(['Ctrl.CPR_BIAS_ADJ'], ['b-'], 'Bias  '+Title, xlims)
    subplot(322)
    overplot_zoom(['Ctrl.CPR_REF', 'Ctrl.CPR'], ['b-', 'g-'], 'CPR', xlims)
    subplot(323)
    overplot_zoom(['Ctrl.CMODE_WF', 'Ctrl.CMODE_A8'], ['k-', 'r-'], 'CMODE', xlims)
    subplot(324)
    overplot_zoom(['Ctrl.A8_POS_REF', 'Ctrl.A8_POS', 'Ctrl.A8_FLOOR'], ['b-', 'g-', 'c--'], 'A8', xlims)
    subplot(325)
    overplot_zoom(['Cmp020SM.SMW', 'Cmp025SM.SMW'], ['r-', 'b-'], 'SM', xlims)
    Fn_q100 = PerfUnInst.Fn; Fn_q100.values = Fn_q100.values/100;
    subplot(326)
    overplot_zoom(['Ctrl.PLA', 'Fn_q100'], ['k-', 'g-'], 'PLA', xlims)

    figs($+1) = figure("Figure_name", 'Null  '+Title, "Position", [440,180,1000,660]); clf();
    subplot(321)
    overplot_zoom(['Ctrl.A8_POS_REF', 'Ctrl.A8_POS', 'Ctrl.A8_FLOOR'], ['b-', 'g-', 'c--'], 'A8  '+Title, xlims)
    subplot(322)
    overplot_zoom(['Ctrl.A8_ERR', 'Ctrl.A8_GAIN'], ['b-', 'g-'], 'Gain', xlims)
    subplot(323)
    A8_NullDmd = Ctrl.A8_TM_NULL_SHIFT; A8_NullDmd.values = Ctrl.A8_ERR.values.*Ctrl.A8_GAIN.values;
    overplot_zoom(['A8_NullDmd', 'Ctrl.A8_TM_NULL_SHIFT', 'Ctrl.A8P_STK_WATCH'], ['b-', 'g-', 'k-'], 'Loop', xlims)
    subplot(324)
    ylims = [-10 10];
    overplot_zoom(['Ctrl.A8P_OP_STK_', 'Ctrl.A8P_CL_STK_', 'Ctrl.A8P_QUIET_', 'Ctrl.A8P_STK_WATCH'], ['r-', 'r--', 'b-','g-'], 'Status', xlims, ylims)
