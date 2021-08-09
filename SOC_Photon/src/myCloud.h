#ifndef my_cloud_h
#define my_cloud_h

int particleHold(String command);
int particleSet(String command);
void gotWeatherData(const char *name, const char *data);
void getWeather(void);
void publish1(void);
void publish2(void);
void publish3(void);
void publish4(void);

#endif
