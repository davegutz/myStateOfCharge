#ifndef _myInsolation_H
#define _myInsolation_H

enum Conditions {FAIR, RAIN, OVERCAST, MOSTLY_CLOUDY, PARTLY_CLOUDY, CLEAR, A_FEW_CLOUDS,
                FOG_MIST, LIGHT_SNOW_FOG_MIST, RAIN_FOG_MIST,LIGHT_RAIN_FOG_MIST, HAZE,
                MIST, LIGHT_SNOW, SNOW, HEAVY_SNOW, UNKNOWN};

class Insolation
{
public:
  Insolation();
  Insolation(const double area, const double reflectivity, const float gmt);
  ~Insolation();
  // operators
  // functions
  double cover() { return(cover_); };
  void getVisibility(const String visStr);
  void getWeather(const String weatherStr);
  double turbidity() { return(turbidity_); };
  double visibility() { return(visibility_); };
  Conditions the_weather() { return(the_weather_); };
  String weatherStr() { return(weatherStr_); };
  double solar_heat();
protected:
  double area_;   // Area illuminated, ft^2
  double cover_;  // Fractional pass of solar from sky coverage
  float GMT_;     // GMT offset, hours.  + is E.
  double obscure_;        // Fractional pass of solar from obscurity/turbidity
  double reflectivity_;   // Fractional reflectivity of energy bounced off wall and rejected (1-ref is used)
  Conditions the_weather_;// From weather station, enum
  double turbidity_;      // Obscurity of air, used to scale solar
  double visibility_;     // Visibility from weather station, miles
  String visStr_;
  String weatherStr_;
};


#endif