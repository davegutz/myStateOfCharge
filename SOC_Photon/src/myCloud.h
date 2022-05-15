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
  float Vbatt;
  float Tbatt;
  float Vshunt;
  float Ishunt;
  float Wshunt;
  float Vshunt_amp;
  float Ishunt_amp_cal;
  float Vshunt_noamp;
  float Ishunt_noamp_cal;
  float T;
  float voc;
  float voc_filt;
  float vsat;
  boolean sat;
  float Tbatt_filt;
  int num_timeouts;
  float tcharge;
  float amp_hrs_remaining_ekf;
  float amp_hrs_remaining_wt;
  float soc_model;
  float soc;
  float soc_ekf;
  float soc_wt;
  float vdyn;
  float voc_ekf;
  float y_ekf;
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
