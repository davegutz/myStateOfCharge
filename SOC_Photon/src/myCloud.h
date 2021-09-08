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
  double VOC_free;
  double Vbatt_filt;
  double Vbatt_filt_obs;
  double Tbatt_filt;
  double Vshunt_filt;
  double Ishunt_filt;
  double Ishunt_filt_obs;
  double Wshunt_filt;
  int numTimeouts;
  double socu_solved;
  double socu_free;
  double Vbatt_model;
  double Vbatt_model_filt;
  double Vbatt_model_solved;
  double tcharge;
};

void publish1(void);
void publish2(void);
void publish3(void);
void publish4(void);
void publish_particle(unsigned long now, Wifi *wifi, const boolean enable_wifi);
void assignPubList(Publish* pubList, const unsigned long now, const String unit, const String hmString,
  const double controlTime, struct Sensors* sen, const int numTimeouts,
  Battery* myBatt_solved, Battery* myBatt_free, Battery* myBatt);

#endif
