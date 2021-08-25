#ifndef my_cloud_h
#define my_cloud_h

#include "mySubs.h"

// Wifi
struct Wifi
{
  unsigned int lastAttempt;
  unsigned int lastDisconnect;
  bool connected = false;
  bool blynk_started = false;
  bool particle_connected_last = false;
  bool particle_connected_now = false;
  Wifi(void) {}
  Wifi(unsigned int lastAttempt, unsigned int lastDisconnect, bool connected, bool blynk_started,
        bool particle_connected)
  {
    this->lastAttempt = lastAttempt;
    this->lastDisconnect = lastDisconnect;
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
  String hmString;
  double controlTime;
  double Vbatt;
  double Tbatt;
  double Vshunt;
  double Ishunt;
  double Wshunt;
  double T;
  int I2C_status;
  double Vbatt_filt;
  double Vbatt_filt_obs;
  double Tbatt_filt;
  double Vshunt_filt;
  double Ishunt_filt;
  double Ishunt_filt_obs;
  double Wshunt_filt;
  int numTimeouts;
  double SOC;
  double SOC_tracked;
  double SOC_solved;
  double Vbatt_model;
  double Vbatt_model_filt;
  double Vbatt_model_tracked;
  double Vbatt_model_solved;
};

void publish1(void);
void publish2(void);
void publish3(void);
void publish4(void);
void publish_particle(unsigned long now, Wifi *wifi);
void assignPubList(Publish* pubList, const unsigned long now, const String unit, const String hmString,
  const double controlTime, struct Sensors* sen, const int numTimeouts,
  Battery* myBatt, Battery* myBatt_tracked, Battery* myBatt_solved);

#endif
