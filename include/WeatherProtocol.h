#pragma once


enum
{
    WEATHER_TEMPERATURE_BIT = 1 << 0,
    WEATHER_HUMIDITY_BIT = 1 << 1,
    WEATHER_PRESSURE_BIT = 1 << 2,
    WEATHER_PM_BIT = 1 << 3,
    WEATHER_BATT_BIT = 1 << 4,
};

#pragma pack(push, 1)

struct WeatherHeader
{
    unsigned char m_DataIncluded; //bitfield of WEATHER_*_BITs
    //depending on which bits are set, the following structures follow in this order
};

struct TemperatureData
{
  short m_Temperature;  // in hundredths of degrees C
};

struct HumidityData
{
  unsigned short m_Humidity;  // in tenths of percent
};

struct PressureData
{
  unsigned int m_Pressure;  // in hundredths of pascals
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
    int m_Milliamps; //in milliamps
};

#pragma pack(pop)
