#pragma once


enum
{
    WEATHER_TEMP_BIT = 1 << 0,
    WEATHER_CO2_BIT = 1 << 1,
    WEATHER_PM_BIT = 1 << 2,
    WEATHER_BATT_BIT = 1 << 3,
    WEATHER_TEMP_ONLY_BIT = 1 << 4,
    WEATHER_HUMID_BIT = 1 << 5,
    WEATHER_PRESSURE_BIT = 1 << 6,
};

#pragma pack(push, 1)

struct WeatherHeader
{
    unsigned char m_DataIncluded; //bitfield of WEATHER_*_BITs
    //depending on which bits are set, the following structures follow in this order
};

struct TemperatureData
{
  short m_Temperature;       // in hundredths of degrees
  unsigned short m_Humidity;  // in tenths of percent
  unsigned int m_Pressure;   // in hundredths of pascals
};

struct CO2Data
{
  unsigned short m_PPM;
};

struct PMData
{
  //all of these in ug/m3
  unsigned short m_10;
  unsigned short m_2_5;
  unsigned short m_0_1;
};

struct BatteryData
{
    unsigned int m_Voltage; //in hundredths of volts
};

#pragma pack(pop)
