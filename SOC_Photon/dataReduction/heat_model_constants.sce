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


// heat_model_constants.sce
function [M, C] = heat_model_define()
    M.R4 = 22.5;        // Resistance of R4 duct insulation, F-ft^2/BTU
    M.R8 = 45;          // Resistance of R8 duct insulation, F-ft^2/BTU
                        // 5.68 F-ft^2/(BTU/hr).   R8 = 8*5.68 = 45 F-ft^2/(BTU/hr)
    M.R12 = 68;         // Resistance of R12 duct insulation, F-ft^2/(BTU/hr)
    M.R16 = 90;         // Resistance of R16 duct insulation, F-ft^2/(BTU/hr)
    M.R32 = 180;        // Resistance of R32 wall insulation, F-ft^2/(BTU/hr)
    M.R64 = 360;        // Resistance of R64 wall insulation, F-ft^2/(BTU/hr)
    M.Adli = 9;         // Surface area inner muffler box, ft^ft
    M.Adlo = 16;        // Surface area inner muffler box, ft^ft
    M.Adsi = 40;        // Surface area inner duct, ft^ft
    M.Adso = 150/2;     // Surface area inner duct, ft^ft  (half buried)
    M.Cpl = 0.2;        // Heat capacity of muffler box, BTU/lbm/F
    M.Cps = 0.4;        // Heat capacity of duct insulation, BTU/lbm/F
    M.Mdl = 50;         // Mass of muffler box, lbm (50)
    M.Mds = 20;         // Mass of duct, lbm (100)
    M.hf = 1;           // TBD.  Rdf = hf*log10(mdot); Boundary layer heat resistance forced convection, BTU/hr/ft^2/F
                // NOTE:  probably need step data from two different flow conditions to triangulate this value.
    // Specific to simple model
    M.Rwall = M.R4;
    M.Cpa = 0.23885;            // Heat capacity of dry air at 80F, BTU/lbm/F (1.0035 J/g/K)
    M.Cpw = 0.2;                // Heat capacity of walls, BTU/lbm/F
    M.Aw = 12*12 + 7*12*3;      // Surface area room walls and ceiling, ft^2
    M.Mw = 1000;                // Mass of room ws and ceiling, lbm (1000)
    M.hi = 1.4;                 // Heat transfer resistance inside still air, BTU/hr/ft^2/F.  Approx industry avg
    M.ho = 4.4;                 // Heat transfer resistance outside still air, BTU/hr/ft^2/F.  Approx industry avg
    M.rhoa = .0739;             // Density of dry air at 80F, lbm/ft^3
    M.mua = 0.04379;            // Viscosity air, lbm/ft/hr
    M.Mair = 8*12*12 * M.rhoa;  // Mass of air in room, lbm
    M.Duct_temp_drop = 7;       // Observed using infrared thermometer, F (7)
    M.Qlk = 500;                // Model alignment heat loss, BTU/hr (800)
    M.Qlkd = 0;                 // Model alignment heat loss, BTU/hr (800)
    M.mdotl_incr = 360;         // Duct long term heat soak, s (360) 
    M.mdotl_decr =  90;         // Duct long term heat soak, s (90)    data match activities
    M.Rsa = 1/M.hi/M.Aw + M.Rwall/M.Aw + 1/M.ho/M.Aw;
    M.Rsai = 1/M.hi/M.Aw;
    M.Rsao = M.Rwall/M.Aw + 1/M.ho/M.Aw;
    M.t1 = 400;                 // mdot transition to shutoff convetcion
    M.t2 = 700;                 // mdot transition to shutoff convetcion
    M.Gconv = 60;               // Convection gain, BTU/hr/F
    M.Glk = 50;                 // Leak temp coeff
    M.Glkd = 100;               // Duct leak temp coeff
    M.Tk = 68;                  // Kitchen temperature, F
    M.min_tdso = 68;            // Min duct discharge temp, F
    M.t_door_close = -%inf;
    M.t_door_crack = -%inf;
    M.t_door_open = -%inf;
    M.t_sys_off = -%inf;
    M.t_sys_on = -%inf;
    M.Smdot = 1.0;              // Scale duct flow
    // Control law
    C.DB = 0.3;
    C.G = 0.03;
    C.tau = 600;
endfunction


function [M, C] = heat_model_init(M, C)
    M.time = zeros(B.N, 1);
    M.dt = zeros(B.N, 1);
    M.cmd = zeros(B.N, 1);
    M.cfm = zeros(B.N, 1);
    M.OAT = zeros(B.N, 1);
    M.Qai = zeros(B.N, 1);
    M.Qao = zeros(B.N, 1);
    M.Qwi = zeros(B.N, 1);
    M.Qwo = zeros(B.N, 1);
    M.Ta = zeros(B.N, 1);
    M.TaDot = zeros(B.N, 1);
    M.Tdso = zeros(B.N, 1);
    M.Tp = zeros(B.N, 1);
    M.Tw = zeros(B.N, 1);
    M.TwDot = zeros(B.N, 1);
    M.Qmatch = zeros(B.N, 1);
    M.Qconv = zeros(B.N, 1);
    M.mdot = zeros(B.N, 1);
    M.dQa = zeros(B.N, 1);
    M.Tass = zeros(B.N, 1);
    M.Twss = zeros(B.N, 1);
    M.mdrate = zeros(B.N, 1);
    M.lstate = zeros(B.N, 1);
    M.rstate = zeros(B.N, 1);
    M.mdot_nmp = zeros(B.N, 1);
    M.Qleak = zeros(B.N, 1);
    M.Qduct = zeros(B.N, 1);
    M.QleakD = zeros(B.N, 1);
//    M.a = zeros(B.N, 2, 2);
//    M.b = zeros(B.N, 2, 3);
//    M.c = zeros(B.N, 1, 2);
//    M.e = zeros(B.N, 1, 3);
    M.duct_pole = zeros(B.N, 1);
    M.slow_room_pole = zeros(B.N, 1);
    M.fast_room_pole = zeros(B.N, 1);
    C.duty = zeros(B.N, 1);
    C.integ = zeros(B.N, 1);
    C.prop = zeros(B.N, 1);
    M.Tkit = zeros(B.N, 1);
endfunction
