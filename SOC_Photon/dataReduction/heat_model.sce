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

//// Heat model
//// Tdl temp of large duct mass Mdl, considered to be the muffler box, F
//// Tds temp of small duct mass Mds, considered to be the lenght of 6" duct, F
//// Rdl coefficient of thermal transfer, BTU/hr/F/ft^2
//// Rds coefficient of thermal transfer, BTU/hr/F/ft^2
//// Mdl, mass of muffler box, lbm
//// Mds, mass of duct, lbm
//
//// Will be tuned to match data.   Using physics-based form
//// to get the structure correct.   Some numbers are approximate.
//function C = heat_proto(debug)
//// Major assumptions:
////      Attic temperature same as OAT
////      Fan/duct airflow and pressure instantaneous response to cmd
////      Air at 80F.   Apply 75F models to 80F
////      Uniform mixing of duct air into room
////      Bulk temperatures of air in muffler and duct are the supply temps
////      Neglect radiation (we're only trying to get the shape right, then match)
////      Insulation mass Tb splits the R-value between inside and outside transfer
////      Neglect convection draft from Kitchen that occurs at very low mdot
//    Aw = 12*12 + 7*12*3;   // Surface area room walls and ceiling, ft^2
//    Adli = 9;   // Surface area inner muffler box, ft^ft
//    Adlo = 16;  // Surface area inner muffler box, ft^ft
//    Adsi = 40;  // Surface area inner duct, ft^ft
//    Adso = 150/2; // Surface area inner duct, ft^ft  (half buried)
//    Cpa = 0.23885; // Heat capacity of dry air at 80F, BTU/lbm/F (1.0035 J/g/K)
//    Cpl = 0.2;  // Heat capacity of muffler box, BTU/lbm/F
//    Cps = 0.4;  // Heat capacity of duct insulation, BTU/lbm/F
//    Cpw = 0.2;  // Heat capacity of walls, BTU/lbm/F
//    Mw = 1000;  // Mass of room ws and ceiling, lbm (1000)
//    Mdl = 50;   // Mass of muffler box, lbm (50)
//    Mds = 20;  // Mass of duct, lbm (100)
//    hf = 1;     // TBD.  Rdf = hf*log10(mdot);    // Boundary layer heat resistance forced convection, BTU/hr/ft^2/F
//                // NOTE:  probably need step data from two different flow conditions to triangulate this value.
//    hi = 1.4;   // Heat transfer resistance inside still air, BTU/hr/ft^2/F.  Approx industry avg
//    ho = 4.4;   // Heat transfer resistance outside still air, BTU/hr/ft^2/F.  Approx industry avg
//    rhoa = .0739;    // Density of dry air at 80F, lbm/ft^3
//    mua = 0.04379;  // Viscosity air, lbm/ft/hr
//    Mair = 8*12*12 * rhoa; // Mass of air in room, lbm
//    
//    R8 = 45;  // Resistance of R8 duct insulation, F-ft^2/BTU
//    // 5.68 F-ft^2/(BTU/hr).   R8 = 8*5.68 = 45 F-ft^2/(BTU/hr)
//    R16 = 90;  // Resistance of R8 duct insulation, F-ft^2/(BTU/hr)
//    R22 = 125;  // Resistance of R22 wall insulation, F-ft^2/(BTU/hr)
//    R32 = 180;  // Resistance of R22 wall insulation, F-ft^2/(BTU/hr)
//    R64 = 360;  // Resistance of R22 wall insulation, F-ft^2/(BTU/hr)
//    
//    //mdotd     Mass flow rate of air through duct, lbm/sec
//    // Pfan     Fan pressure, in H20 at 75F, assume = 80F
//    // Qduct    Duct = Fan airflow, CFM at 75F, assume = 80F
//    // Tp       Plenum temperature, F
//    // Ta       Sunshine Room temperature, F
//    // Tdo      Duct discharge temp, F
//    // Tbs      Small duct bulk air temp, F
//    // Tbl      Muffler bulk air temp, F
//    
//    
//    // Rdf model:
//    //  R ~ 2 - 100  BTU/hr/ft^2/F
//    
//    // Loop for time transient
//    dt = 10;   // sec time step
//    Tp = 80;  // Duct supply, plenum temperature, F
//    plotting = 1;
//    run_name = 'heat_model';
//    
//    //pause
//    
//    // Initialize
//    OAT = 30;   // Outside air temperature, F
//    cmdi = 90;
//    cmdf = 100;
//    
//    Rsl = 1/hi/Adli + R22/2/Adli + R22/2/Adlo + 1/ho/Adlo;
//    Rsli = 1/hi/Adli + R22/2/Adli;
//    Rslo = R22/2/Adlo + 1/ho/Adlo;
//    Hdso = ho*Adso;
//    
//    Rsa = 1/hi/Aw + R22/Aw + 1/ho/Aw;
//    Rsai = 1/hi/Aw;
//    Rsao = R22/Aw + 1/ho/Aw;
//    Hai = hi*Aw;
//    Hao = ho*Aw;
//    
//    C = "";
//    for time = 0:dt:3600*16
//        
//        if time<1000 then, cmd = cmdi; else cmd = cmdf;  end
//        [cfm, mdotd, hduct] = flow_model(cmd, rhoa, mua);
//        Tdli = Tp;
//        Tbl = Tp;
//        Hdli = hduct*Adli;
//        Hdsi = hduct*Adsi;
//        Rss = 1/hduct/Adsi + R8/2/Adsi +R8/2/Adso + 1/ho/Adso;
//        Rssi = 1/hduct/Adsi + R8/2/Adsi;
//        Rsso = R8/2/Adso + 1/ho/Adso;
//        Hdlo = ho*Adlo;
//    
//        // Init logic
//        if time<1e-12, then
//    
//            // Exact solution muffler box
//            Tdli = Tp;
//            Tbl = (2*mdotd*Cpa*Rsl*Tdli + OAT) / (2*mdotd*Cpa*Rsl + 1);
//            Qdl = (Tbl - OAT) / Rsl;
//            Tml = Tbl - Qdl * Rsli;
//            Tmli = Tbl - Qdl / Hdli;
//            Tmlo = OAT + Qdl / Hdlo;
//            Tdlo = 2*Tbl - Tdli;
//            Qdla = (Tdli - Tdlo)*mdotd*Cpa;  // BTU/hr
//                    
//            // Exact solution duct
//            Tdsi = Tdlo;
//            Tbs = (2*mdotd*Cpa*Rss*Tdsi + OAT) / (2*mdotd*Cpa*Rss + 1);
//            Qds = (Tbs - OAT) / Rss;
//            Tms = Tbs - Qds * Rssi;
//    //        Tmsi = Tbs - Qds / Hdsi;
//    //        Tmso = OAT + Qds / Hdso;
//            Tdso = 2*Tbs - Tdsi;
//            Qdsa = (Tdsi - Tdso)*mdotd*Cpa;  // BTU/hr
//    
//            // Exact solution room
//            Ta = (mdotd*Cpa*Rsa*Tdso + OAT) / (mdotd*Cpa*Rsa + 1);
//            Qa = (Ta - OAT) / Rsa;
//            Tw = Ta - Qa * Rsai;
//            Tmai = Ta - Qa / Hai;
//            Tmao = OAT + Qa / Hao;
//            Qai = Tdso*mdotd*Cpa;
//            Qao = Ta*mdotd*Cpa;
//    
//        end
//    
//        // Flux
//        Tbl = (2*mdotd*Cpa*Rsli*Tdli + Tml) / (2*mdotd*Cpa*Rsli + 1);
//        Qdli = (Tbl - Tml) / Rsli;
//        Qdlo = (Tml - OAT) / Rslo;
//        Tdlo = 2*Tbl - Tdli;
//    
//        Tdsi = Tdlo;
//        Tbs = (2*mdotd*Cpa*Rssi*Tdsi + Tms) / (2*mdotd*Cpa*Rssi + 1);
//        Qdsi = (Tbs - Tms) / Rssi;
//        Qdso = (Tms - OAT) / Rsso;
//        Tdso = 2*Tbs - Tdsi;
//    
//        Qai = Tdso * Cpa * mdotd;
//        Qao = Ta * Cpa * mdotd;
//    
//        Qwi = (Ta - Tw) / Rsai;
//        Qwo = (Tw - OAT) / Rsao;
//    
//        // Derivatives
//        TmlDot = (Qdli - Qdlo)/3600 / (Cpl * Mdl);
//        TmsDot = (Qdsi - Qdso)/3600 / (Cps * Mds);
//        TaDot = (Qai - Qao - Qwi)/3600 / (Cpa * Mair);
//        TwDot = (Qwi - Qwo)/3600 / (Cpw * Mw);
//    
//        // Store results
//        if debug>2 then
//            if time==0 then, printf('  time,      cmd,   hduct,   cfm,    mdotd,   Tp,      Tbl,     Tml,     Tbs,     Tms,     Tdso,    Ta,        Tw,      OAT,\n'); end
//            printf('%7.1f, %7.1f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f, %7.3f,\n',...
//                time, cmd, hduct, cfm, mdotd, Tp, Tbl, Tml, Tbs, Tms, Tdso, Ta, Tw, OAT);
//        end
//



//        C.time($+1) = time;
//        C.cmd($+1) = cmd;
//        C.hduct($+1) = hduct;
//        C.cfm($+1) = cfm;
//        C.mdotd($+1) = mdotd;
//        C.Tp($+1) = Tp;
//        C.Tbl($+1) = Tbl;
//        C.Tml($+1) = Tml;
//        C.Tbs($+1) = Tbs;
//        C.Tms($+1) = Tms;
//        C.Tdso($+1) = Tdso;
//        C.Tdsi($+1) = Tdsi;
//        C.Ta($+1) = Ta;
//        C.Tw($+1) = Tw;
//        C.OAT($+1) = OAT;
//        C.Qdli($+1) = Qdli;
//        C.Qdlo($+1) = Qdlo;
//        C.Qdsi($+1) = Qdsi;
//        C.Qdso($+1) = Qdso;
//        C.Qai($+1) = Qai;
//        C.Qao($+1) = Qao;
//        C.Qwi($+1) = Qwi;
//        C.Qwo($+1) = Qwo;
//        C.TmlDot($+1) = TmlDot;
//        C.TmsDot($+1) = TmsDot;
//        C.TaDot($+1) = TaDot;
//        C.TwDot($+1) = TwDot;
//        C.QaiMQao($+1) = Qai-Qao;
//        
//        // Integrate
//        Tml = min(max(Tml + dt*TmlDot, OAT), Tbl);
//        Tms = min(max(Tms + dt*TmsDot, OAT), Tbs);
//        Ta = min(max(Ta + dt*TaDot, Tw), Tdso);
//        Tw = min(max(Tw + dt*TwDot, OAT), Ta);
//    
//    
//    end
//endfunction

// Airflow model ECMF-150 6" duct at 50', bends, filters, muffler box
// see ../datasheets/airflow model.xlsx
function [Qduct, mdot, hf, dMdot_dCmd] = flow_model(%cmd, rho, mu);
    // %cmd      Fan speed, %
    // Pfan     Fan pressure, in H20 at 75F
    // mdotd    Duct = Fan airflow, lbm/hr at 75F
    // hf       Forced convection, BTU/hr/ft^2/F
    // rho      Density, lbm/ft^3
    // mu       Viscosity air, lbm/ft/hr
    Qduct = M.Smdot*(-0.005153*%cmd^2 + 2.621644*%cmd);  // CFM
    dQ_dCmd = M.Smdot*(2*(-0.005153)*%cmd + 2.621644);
    mdot = Qduct * rho * 60;   // lbm/hr
    dMdot_dQ = rho * 60;
    Pfan = 5.592E-05*%cmd^2 + 3.401E-03*%cmd - 2.102E-02;  // in H2O
    d = 0.5;   // duct diameter, ft
    Ax = %pi*d^2/4;             // duct cross section, ft^2
    V = Qduct / Ax * 60;        // ft/hr
    Red = rho * V * d / mu;

    hfi = 1.1;
//    log10hfi = 0.804*log10(Red) - 1.72; 

    hfo = 4.4;
//    log10hfo = 0.804*log10(Red) - 1.12;

    log10hf = 0.804*log10(Red) - 1.72;   // Use smaller to start
    // For data fit, adjust the addend
    hf = max(10^(0.804*log10(Red) - 1.72), hfi);
    
    dMdot_dCmd = dQ_dCmd * dMdot_dQ;
endfunction

// Calculate all aspects of heat model
function [M, a, b, c, dMdot_dCmd] = total_model(time, dt, Tp, OAT, %cmd, reset, M, i, B, do_poles);
    // Neglect duct heat effects
    // reset    Flag to indicate initialization

    // Temp lag
    Tdso_raw = max(Tp - M.Duct_temp_drop, M.min_tdso);
    if reset then,
        Tdso = Tdso_raw;
    else
        delta = Tdso_raw - M.Tdso(i-1);
        if delta > 0 then d_td_dt = (delta)/M.mdotl_incr; else d_td_dt = delta/M.mdotl_decr; end  //(240 & 90)
        Tdso = M.Tdso(i-1) + dt*d_td_dt;
    end

    // Inputs.   In testing, close happens before crack happens before open
    [cfm, mdot_raw, hduct, dMdot_dCmd] = flow_model(%cmd, M.rhoa, M.mua);
    flow_blocked = (time<M.t_door_crack & time>M.t_door_close) || ...
                   (time<M.t_door_open & time>M.t_door_close);
    flow_off = (time<M.t_sys_on & time>M.t_sys_off);
    conv_off = flow_off || flow_blocked;
    if  flow_off then,
        mdot_raw = 0;
        cfm = 0;
    end
    otherHeat = B.solar_heat(i);

    // Turn convection on/off
    Sconv = (1-max(min((mdot_raw-M.t1)/(M.t2-M.t1), 1), 0));
    if conv_off then Sconv = 0; end
    
    // Flow filter
    if reset then,
        mdot = mdot_raw;
    else
        delta = mdot_raw - M.mdot(i-1);
        if delta > 0 then d_mdot_dt = (delta)/M.mdotl_incr; else d_mdot_dt = delta/M.mdotl_decr; end  //(240 & 90)
        mdot = M.mdot(i-1) + dt*d_mdot_dt;
    end
    
    // Duct loss
    dTdso_dTp = 1;

    // Kitchen
    Tk = M.Tk;

    // Initialize
    // For linear model assume all these biases and values are constant operating conditions dY_dX = 0
    Qlkd = M.Glkd*(Tp-OAT) + M.Qlkd;
    if mdot_raw==0 then
        Qlkd = 0;
    end
    
    Tass = max(( (mdot_raw*M.Cpa*Tdso - Qlkd + M.Qlk + M.Glk*Tk)*M.Rsa + OAT + Tk*M.Gconv*Sconv*M.Rsa) / (mdot_raw*M.Cpa*M.Rsa + M.Gconv*Sconv*M.Rsa + M.Glk*M.Rsa + 1), OAT);
    Qwiss = (Tass - OAT) / M.Rsa;
    Twss = max(Tass - Qwiss * M.Rsai, OAT);
    Qduct = mdot*M.Cpa*Tdso - Qlkd;
    if reset then,
        Ta = Tass;
        Qai = (Tass - OAT) / M.Rsa;
        Qao = Qai;
        Tw = Twss;
        if force_init_ta then
            Ta = ta_init;
            Tass = Ta;
            Qwiss = (Tass - OAT) / M.Rsa;
            Twss = max(Tass - Qwiss * M.Rsai, OAT);
            Tw = Twss;
        end
        Qlk = M.Glk*(Tk-Ta) + M.Qlk;
    else
        Ta = M.Ta(i-1);
        Tw = M.Tw(i-1);
        Qlk = M.Glk*(Tk-Ta) + M.Qlk;
        Qai = Qduct + Qlk;
        Qao = Ta * M.Cpa * mdot;
    end
    //    Tmai = Ta - Qai / M.Hai;
    //    Tmao = OAT + Qao / M.Hao;
    dQa = Qai - Qao;
    Qwi = (Ta - Tw) / M.Rsai;
    Qwo = (Tw - OAT) / M.Rsao;
    Qconv = (Tk - Ta)*M.Gconv*Sconv;
    // TODO:   check Qmatch out for accuracy
    Qmatch = (B.Ta_Sense(i)*(mdot_raw*M.Cpa*M.Rsa + M.Gconv*Sconv*M.Rsa + M.Glk*M.Rsa + 1) - ((mdot_raw*M.Cpa*Tdso  - Qlkd + M.Qlk + M.Glk*Tk)*M.Rsa + OAT + Tk*M.Gconv*Sconv*M.Rsa) ) / M.Rsa;

    // Derivatives
    dn_TaDot = 3600 * M.Cpa * M.Mair;
    dn_TwDot = 3600 * M.Cpw * M.Mw;
    TaDot = (Qai - Qao - Qwi + Qconv + otherHeat) / dn_TaDot;
    TwDot = (Qwi - Qwo) / dn_TwDot;
    
    // Integrate
    Ta = min(max(Ta + dt*TaDot, OAT), Tdso);
    Tw = min(max(Tw + dt*TwDot, OAT), Tdso);
    
//if reset | time>-17500 then, pause; end
//if time>-20000 then, pause; end
//if time>-17000 then, pause; end
//if time>-10000 then, pause; end
//if time>-18000 then, pause; end
//if time>-25000 then, pause; end


    // Consolidate the linear model
    // states:  {Ta  Tw}
    // inputs:  {mdot Tdso OAT}
    // outputs: {Ta}
    // TODO: add Gconv Glk and Glkd
    if do_poles then,
        dQai_dMdot = M.Cpa * Tdso;
        dQai_dTdso = M.Cpa * mdot;
        dQao_dMdot = M.Cpa * Ta;
        dQao_dTa = M.Cpa * mdot;
        dQwi_dTa = 1/M.Rsai;
        dQwi_dTw = -1/M.Rsao;
        dQwo_dTw = 1/M.Rsai;
        dQwo_dOAT = -1/M.Rsao;
    
        dTaDot_dQai = 1/dn_TaDot;
        dTaDot_dQao = -1/dn_TaDot;
        dTaDot_dQwi = -1/dn_TaDot;
        dTwDot_dQwi = 1/dn_TwDot;
        dTwDot_dQwo = -1/dn_TwDot;
        
        dTaDot_dTa = dTaDot_dQao * dQao_dTa  +  dTaDot_dQwi * dQwi_dTa;
        dTaDot_dTw = dTaDot_dQwi * dQwi_dTw;
        
        dTwDot_dTw = dTwDot_dQwi * dQwi_dTw  +  dTwDot_dQwo * dQwo_dTw;
        dTwDot_dTa = dTwDot_dQwi * dQwi_dTa;
    
        dTaDot_dMdot = dTaDot_dQai * dQai_dMdot  +  dTaDot_dQao * dQao_dMdot;
        dTaDot_dTdso = dTaDot_dQai * dQai_dTdso;
        dTaDot_dOAT = 0;
    
        dTwDot_dMdot = 0;
        dTwDot_dTdso = 0;
        dTwDot_dOAT = dTwDot_dQwo * dQwo_dOAT;
        
        dTa_dTa = 1;
        dTa_dTw = 0;
        
        dTa_dMdot = 0;
        dTa_dTdso = 0;
        dTa_dOAT = 0;
        // states:  {Ta  Tw}
        // inputs:  {mdot Tdso OAT}
        // outputs: {Ta}
        a = [  dTaDot_dTa      dTaDot_dTw;
               dTwDot_dTa      dTwDot_dTw  ];
        b = [  dTaDot_dMdot; dTwDot_dMdot];
        c = [  dTa_dTa         dTa_dTw     ];
        e = [  dTa_dMdot       dTa_dTdso       dTa_dOAT];
    else
        a=1;b=1;c=1;e=1;
    end

    // Save / store
    M.time(i) = time;
    M.dt(i) = dt;
    M.%cmd(i) = %cmd;
    M.cfm(i) = cfm;
    M.OAT(i) = OAT;
    M.Qai(i) = Qai;
    M.Qao(i) = Qao;
    M.Qwi(i) = Qwi;
    M.Qwo(i) = Qwo;
    M.Ta(i) = Ta;
    M.TaDot(i) = TaDot;
    M.Tdso(i) = Tdso;
    M.Tp(i) = Tp;
    M.Tw(i) = Tw;
    M.TwDot(i) = TwDot;
    M.dQa(i) = dQa;
    M.Qmatch(i) = Qmatch;
    M.Qconv(i) = Qconv*Sconv;
    M.Qduct(i) = Qduct;
    M.mdot(i) = mdot;
    M.Tass(i) = Tass;
    M.Twss(i) = Twss;
    M.Qleak(i) = Qlk;
    M.QleakD(i) = Qlkd;
    M.Tkit(i) = Tk;
end
