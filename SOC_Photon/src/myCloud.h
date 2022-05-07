#ifndef my_cloud_h
#define my_cloud_h

#include "Battery.h"

// Wifi
struct Wifi
{
  unsigned int lastAttempt;
  unsigned int last_disconnect;
  boolean connected = false;
  boolean blynk_started = false;
  boolean particle_connected_last = false;
  boolean particle_connected_now = false;
  Wifi(void) {}
  Wifi(unsigned int lastAttempt, unsigned int last_disconnect, boolean connected, boolean blynk_started,
        boolean particle_connected)
  {
    this->lastAttempt = lastAttempt;
    this->last_disconnect = last_disconnect;
    this->connected = connected;
    this->blynk_started = blynk_started;
    this->particle_connected_last = particle_connected;
    this->particle_connected_now = particle_connected;
  }
};

// Publishing
struct Publish
{
  uint32_t now;
  String unit;
  String hm_string;
  double control_time;
  t_float Vbatt;
  t_float Tbatt;
  t_float Vshunt;
  t_float Ishunt;
  t_float Wshunt;
  t_float Vshunt_amp;
  t_float Ishunt_amp_cal;
  t_float Vshunt_noamp;
  t_float Ishunt_noamp_cal;
  boolean curr_sel_noamp; 
  t_float T;
  t_float voc;
  t_float voc_filt;
  t_float vsat;
  boolean sat;
  t_float Tbatt_filt;
  t_float Tbatt_filt_model;
  int num_timeouts;
  t_float tcharge;
  t_float amp_hrs_remaining_ekf;
  t_float amp_hrs_remaining_wt;
  t_float soc_model;
  t_float soc;
  t_float soc_ekf;
  t_float SOC_model;
  t_float SOC;
  t_float SOC_ekf;
  t_float soc_wt;
  t_float SOC_wt;
};

void publish1(void);
void publish2(void);
void publish3(void);
void publish4(void);
void publish_particle(unsigned long now, Wifi *wifi, const boolean enable_wifi);
void assign_publist(Publish* pubList, const unsigned long now, const String unit, const String hm_string,
  const double control_time, struct Sensors* Sen, const int num_timeouts,
  BatteryModel* Sim, BatteryMonitor* Mon);

#endif
