#ifndef my_cloud_h
#define my_cloud_h

#include "Battery.h"

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
  double Tbatt;
  double Vshunt;
  double Ishunt;
  double Wshunt;
  double Vshunt_amp;
  double Ishunt_amp_cal;
  double Vshunt_noamp;
  double Ishunt_noamp_cal;
  boolean curr_sel_amp; 
  double T;
  int I2C_status;
  double VOC;
  double Vbatt_filt;
  double Tbatt_filt;
  double Vshunt_filt;
  double Ishunt_filt;
  int num_timeouts;
  double soc_model;
  double soc;
  double soc_ekf;
  double soc_sat;
  double tcharge;
  double soc_avail;
};

void publish1(void);
void publish2(void);
void publish3(void);
void publish4(void);
void publish_particle(unsigned long now, Wifi *wifi, const boolean enable_wifi);
void assign_publist(Publish* pubList, const unsigned long now, const String unit, const String hm_string,
  const double control_time, struct Sensors* Sen, const int num_timeouts,
  Battery* MyBattModel, Battery* MyBatt);

#endif
