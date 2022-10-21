#ifndef my_cloud_h
#define my_cloud_h

#include "Battery.h"

// Publishing
struct Publish
{
  uint32_t now;
  String unit;
  String hm_string;
  double control_time;
  float Vb;
  float Tb;
  float Vshunt;
  float Ib;
  float Wb;
  float T;
  float Voc_stat;
  float Voc;
  float Voc_filt;
  float Vsat;
  boolean sat;
  float Tb_filt;
  int num_timeouts;
  float tcharge;
  float Amp_hrs_remaining_ekf;
  float Amp_hrs_remaining_soc;
  float soc_model;
  float soc;
  float soc_ekf;
  float dV_dyn;
  float Voc_ekf;
  float y_ekf;
  float ioc;
  float voc_soc;
};

void assign_publist(Publish* pubList, const unsigned long now, const String unit, const String hm_string,
  Sensors* Sen, const int num_timeouts, BatteryMonitor* Mon);

#endif
