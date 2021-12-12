#ifndef my_cloud_h
#define my_cloud_h

#include "mySubs.h"

// Wifi
struct Wifi
{
  unsigned int lastAttempt;
  unsigned int last_disconnect;
  bool connected = false;
  bool blynk_started = false;
  bool particle_connected_last = false;
  bool particle_connected_now = false;
  Wifi(void) {}
  Wifi(unsigned int lastAttempt, unsigned int last_disconnect, bool connected, bool blynk_started,
        bool particle_connected)
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
  double Vbatt;
  double Vbatt_solved;
  double Tbatt;
  double Vshunt;
  double Ishunt;
  double Wshunt;
  double Vshunt_amp;
  double Ishunt_amp;
  double Wshunt_amp;
  double T;
  int I2C_status;
  double VOC_free;
  double VOC_solved;
  double Vbatt_filt;
  double Vbatt_filt_obs;
  double Tbatt_filt;
  double Vshunt_filt;
  double Ishunt_filt;
  double Ishunt_filt_obs;
  double Wshunt_filt;
  double Vshunt_amp_filt;
  double Ishunt_amp_filt;
  double Ishunt_amp_filt_obs;
  double Wshunt_amp_filt;
  int num_timeouts;
  double socu_solved;
  double socu_free;
  double tcharge;
  double soc_avail;
  double socu_model;
};

void publish1(void);
void publish2(void);
void publish3(void);
void publish4(void);
void publish_particle(unsigned long now, Wifi *wifi, const boolean enable_wifi);
void assign_PubList(Publish* pubList, const unsigned long now, const String unit, const String hm_string,
  const double control_time, struct Sensors* Sen, const int num_timeouts,
  Battery* MyBattSolved, Battery* MyBattFree);

#endif
