//
// MIT License
//
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


#ifndef _MY_ROOM_H
#define _MY_ROOM_H

// Duct Model Class
class DuctTherm
{
private:
    String name_;     // Object name label
    double ap_0_;           // Coefficient speed, % to pressure, in H2O
    double ap_1_;           // Coefficient speed, % to pressure, in H2O
    double ap_2_;           // Coefficient speed, % to pressure, in H2O
    double aq_0_;           // Coefficient speed, % to volume flow, cfm
    double aq_1_;           // Coefficient speed, % to volume flow, cfm
    double aq_2_;           // Coefficient speed, % to volume flow, cfm
    double cfm_;            // Duct flow, cfm
    double cpa_;        // Heat capacity of dry air at 80F, BTU/lbm/F
    double duct_dia_;       // Duct diameter, ft
    double duct_temp_drop_; // Observed using infrared thermometer, F (7)
    double fan_speed_;      // Duty (0-255) converted to 0-100, %
    double glkd_;           // Duct heat loss gain, Btu/hr/F
    double mdot_;           // Duct = Fan airflow, lbm/hr 
    double mdot_lag_;       // Lagged duct/fan airflow, lbm/hr 
    double mdot_lag_decr_;  // Duct long term heat soak, s (90)    data match activities
    double mdot_lag_incr_;  // Duct long term heat soak, s (360)   CoolTerm Capture 2021-01-21 14-12-19.xlsx
    double mua_;            // Viscosity air, lbm/ft/hr
    double press_;          // Fan discharge pressure, in H2O
    double Qduct_;          // Net duct heat discharge, Btu/hr
    double Qlkd_;           // Duct heat loss, Btu/hr
    double qlkd_;           // Duct heat loss bias, Btu/hr/F
    double Re_d_;           // Air Reynolds Number
    double rhoa_;           // Density of air, lbm/ft^3
    double Smdot_;          // Duct flow scalar
    double Tdso_;           // Duct flow discharge temperature, F
    double vel_;            // Duct flow, ft/hr
    double Tp_lagged_;      // Tp lagged with heat soak Tp_lag_;
    double flow_model_(const double fan_speed, const double rhoa, const double mua);
public:
    DuctTherm(void);
    DuctTherm(const String name, const double ap_0, const double ap_1, const double ap_2,
        const double aq_0, const double aq_1, const double aq_2, const double cpa,
        const double duct_dia, const double duct_temp_drop,
        const double glkd, const double qlkd, const double mdot_lag_decr, const double mdot_lag_incr,
        const double mua, const double rhoa, const double Smdot);
    void update(const int reset, const double T, const double Tp,  const uint32_t duty, const double OAT);
    double cfm(void){return cfm_;};
    double mdot(void){return mdot_;};
    double mdot_lag(void){return mdot_lag_;};
    double Qduct(void){return Qduct_;};
    double Qlkd(void){return Qlkd_;};
    double Tdso(void){return Tdso_;};
};

// Room Model Class
class RoomTherm
{
private:
    String name_;     // Object name label
    double cpa_;        // Heat capacity of dry air at 80F, BTU/lbm/F
    double dn_tadot_;   // Heat capacitance of air, (BTU/hr)  / (F/sec)
    double dn_twdot_;   // Heat capacitance of air, (BTU/hr)  / (F/sec)
    double dQa_;        // Net duction heat, BTU/hr
    double Gconv_;      // Convection gain, (BTU/hr) / F
    double glk_;        // Unknown heat loss coefficient, (BTU/hr) / F
    double qconv_;      // Model convection heat gain fudge factor, BTU/hr
    double Qlk_;        // Total unknown heat loss, BTU/hr
    double qlk_;        // Unknown heat loss bias, BTU/hr
    double rsa_;        // Effective resistance of air,  F-ft^2/(BTU/hr)
    double rsai_;       // Resistance air to wall,  F-ft^2/(BTU/hr)
    double rsao_;       // Resistance wall to OAT, F-ft^2/(BTU/hr)
    double Ta_;         // Air temp, F
    double Tk_;         // Kitchen temp, F
    double trans_conv_high_;    // High transition point of mdot, scalar becomes 0, lbm/hr at 75F
    double trans_conv_low_;     // Low transition point of mdot, scalar begins to incr from 1 to 0, lbm/hr at 75F
    double Tw_;         // Outside wall temp, F
public:
    RoomTherm(void);
    RoomTherm(const String name, const double cpa, const double dn_tadot, const double dn_twdot, const double Gconv,
    const double glk, const double qlk, const double rsa, const double rsai, const double rsao,
    const double trans_conv_low, const double trans_conv_high);
    double Qlk(void){return Qlk_;};
    void update(const int reset, const double T, const double Qduct, const double mdot, const double mdot_lag,
        const double Tk, const double OAT, const double otherHeat, const double set);
    double Ta(void){return Ta_;};
    double Tw(void){return Tw_;};
};


#endif