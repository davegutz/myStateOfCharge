/*
 * Class myInsolation
  * Description:
  * Calculate total insolation at time of day in weather from 
  * a surface
  * 
  * By:  Dave Gutz February 2021
  * 16-Feb-2021   New
  * 
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
*/


#include "application.h" // String, int8_t, etc
#include "myInsolation.h"
#include <math.h>

extern int8_t debug;
const double pi = 3.1415926;

// Constructors
Insolation::Insolation()
  : area_(0), cover_(0), GMT_(0), obscure_(0), reflectivity_(0),
  the_weather_(FAIR), turbidity_(0), visibility_(0), visStr_(""), weatherStr_("")
{}
Insolation::Insolation(const double area, const double reflectivity, const float gmt)
  : area_(area), cover_(0), GMT_(gmt), obscure_(1), reflectivity_(reflectivity), 
  the_weather_(FAIR), turbidity_(0), visibility_(0), visStr_(""), weatherStr_("")
{}

// Functions
// Visibility
void Insolation::getVisibility( const String visStr )
{
  if ( visStr!= "" )
  {
    visStr_ = visStr;
    visibility_ = atof(visStr);
  }
  if ( visibility_>8.0 ) turbidity_ = 2.2;
  else if ( visibility_>4.0 ) turbidity_ = 4;
  else if ( visibility_>2.0 ) turbidity_ = 8;
  else if ( visibility_>1.2 ) turbidity_ = 16;
  else if ( visibility_>0.8 ) turbidity_ = 32;
  else if ( visibility_>0.5 ) turbidity_ = 64;
  else turbidity_ = 128;
  // Can't find references.   TODO
  obscure_ = 1.;
}

// Weather
void Insolation::getWeather( const String weatherStr )
{
  if ( weatherStr!= "" )
  {
    weatherStr_ = weatherStr;
    if ( weatherStr=="Rain" ) the_weather_ = RAIN;
    else if ( weatherStr=="Fair" ) the_weather_ = FAIR;
    else if ( weatherStr=="Overcast" ) the_weather_ = OVERCAST;
    else if ( weatherStr=="Mostly Cloudy" ) the_weather_ = MOSTLY_CLOUDY;
    else if ( weatherStr=="Partly Cloudy" ) the_weather_ = PARTLY_CLOUDY;
    else if ( weatherStr=="Clear" ) the_weather_ = CLEAR;
    else if ( weatherStr=="A Few Clouds" ) the_weather_ = A_FEW_CLOUDS;
    else if ( weatherStr=="Fog/Mist" ) the_weather_ = FOG_MIST;
    else if ( weatherStr=="Rain Fog/Mist" ) the_weather_ = RAIN_FOG_MIST;
    else if ( weatherStr=="Light Rain Fog/Mist" ) the_weather_ = LIGHT_RAIN_FOG_MIST;
    else if ( weatherStr=="Haze" ) the_weather_ = HAZE;
    else if ( weatherStr=="Mist" ) the_weather_ = MIST;
    else if ( weatherStr=="Light Snow" ) the_weather_ = LIGHT_SNOW;
    else if ( weatherStr=="Light Snow Fog/Mist" ) the_weather_ = LIGHT_SNOW_FOG_MIST;
    else if ( weatherStr=="Snow" ) the_weather_ = SNOW;
    else if ( weatherStr=="Heavy Snow" ) the_weather_ = HEAVY_SNOW;
    else the_weather_ = UNKNOWN;
  }
  switch ( the_weather_ )
  {
    case FAIR: case CLEAR: cover_ = 1.0;
      break;
    case PARTLY_CLOUDY: case A_FEW_CLOUDS: case HAZE: cover_ = 0.85;
      break;
    case RAIN: case OVERCAST: case MOSTLY_CLOUDY: case FOG_MIST: case LIGHT_SNOW_FOG_MIST:
          case RAIN_FOG_MIST: case LIGHT_RAIN_FOG_MIST: case MIST: case LIGHT_SNOW: case SNOW:
          case HEAVY_SNOW: case UNKNOWN: cover_ = 0.7;
      break;
    default: cover_ = 0.7;
      break;
  }
}

// Solar heating
double Insolation::solar_heat()
{
  // Current time
  Time.zone(GMT_);
  time32_t currentTime = Time.now();
  uint8_t month = Time.month(currentTime);
  uint8_t day = Time.day(currentTime);
  uint8_t hour = Time.hour(currentTime);
  uint8_t minute = Time.minute(currentTime);
  double DT = double(hour) + double(minute)/60;   // Decimal time
  double DM = double(month-1) + double(day)/30;   // Decimal month
  // See ../../Sky Model/SolarModelDemo.sce and CR674_1.xlsm
  double I = 0.317 * 880 * cover_ * obscure_ * reflectivity_ *
           max( cos(1.2*(12-DT)*pi/12) * (0.75+0.25*cos((6-DM)*pi/6)), 0);
  // 0.317 Btu/hr/fft^2 / W/m^2
  return ( I*area_*(1-reflectivity_) );
}