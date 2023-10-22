#include <Arduino.h>
#include <WiFi.h>
#include "creds.h"
#include "debug.h"
#include "../include/IngestProtocol.h"
#include "../include/WeatherProtocol.h"

IPAddress local_IP(192, 168, 1, 222);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

const unsigned short IngestPort = 7777;
const unsigned short ServerPort = 7788;
WiFiServer m_IngestSocket;
WiFiServer m_ServerSocket;

WeatherHeader m_WeatherHeader;
TemperatureData m_TemperatureData;
CO2Data m_CO2Data;
PMData m_PMData;
BatteryData m_BatteryData;

void setup()
{
  Serial.begin(115200);

  m_WeatherHeader.m_DataIncluded = 0; //we have no data!!

  //connect to WiFi
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(MY_SSID, MY_WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.println("Waiting for wifi to connect...");
  }
  Serial.println("Connected to WiFi!");

  m_IngestSocket.begin(IngestPort);
  m_ServerSocket.begin(ServerPort);
}

void IngestWeatherData(WiFiClient& client)
{
  WeatherHeader header;
  int numBytes = client.readBytes((char*)&header, sizeof(header));
  if (numBytes != sizeof(header))
  {
    DebugPrint("No data included?\n");
    return;
  }

  if ((header.m_DataIncluded & WEATHER_TEMP_BIT) != 0)
  {
    TemperatureData data;
    int numBytes = client.readBytes((char*)&data, sizeof(data));
    if (numBytes != sizeof(data))
    {
      DebugPrintf("Not enough bytes for temp data, got %d, expecting %d\n", numBytes, sizeof(data));
      return;
    }
    m_TemperatureData = data;
    DebugPrintf("Got temperature stuff: %.1f F, %.1f%%, %.2f\n", m_TemperatureData.m_Temperature / 100.0 * 9 / 5 + 32, m_TemperatureData.m_Humidity / 10.0, m_TemperatureData.m_Pressure / 100.0);
    m_WeatherHeader.m_DataIncluded |= WEATHER_TEMP_BIT;
  }

  if ((header.m_DataIncluded & WEATHER_CO2_BIT) != 0)
  {
    CO2Data data;
    int numBytes = client.readBytes((char*)&data, sizeof(data));
    if (numBytes != sizeof(data))
    {
      DebugPrintf("Not enough bytes for co2 data, got %d, expecting %d\n", numBytes, sizeof(data));
      return;
    }
    m_CO2Data = data;
    DebugPrintf("Got CO2 stuff: %hu ppm\n", m_CO2Data.m_PPM);
    m_WeatherHeader.m_DataIncluded |= WEATHER_CO2_BIT;
  }

  if ((header.m_DataIncluded & WEATHER_PM_BIT) != 0)
  {
    PMData data;
    int numBytes = client.readBytes((char*)&data, sizeof(data));
    if (numBytes != sizeof(data))
    {
      DebugPrintf("Not enough bytes for pm data, got %d, expecting %d\n", numBytes, sizeof(data));
      return;
    }
    m_PMData = data;
    DebugPrintf("Got PM stuff: pm10: %hhu, pm2.5: %hhu, pm0.1: %hhu\n", m_PMData.m_10, m_PMData.m_2_5, m_PMData.m_0_1);
    m_WeatherHeader.m_DataIncluded |= WEATHER_PM_BIT;
  }

  if ((header.m_DataIncluded & WEATHER_BATT_BIT) != 0)
  {
    BatteryData data;
    int numBytes = client.readBytes((char*)&data, sizeof(data));
    if (numBytes != sizeof(data))
    {
      DebugPrintf("Not enough bytes for battery data, got %d, expecting %d\n", numBytes, sizeof(data));
      return;
    }
    m_BatteryData = data;
    DebugPrintf("Got battery stuff: %.2f Volts\n", m_BatteryData.m_Voltage / 100.0);
    m_WeatherHeader.m_DataIncluded |= WEATHER_BATT_BIT;
  }
}

void DoIngest(WiFiClient& client)
{
  unsigned char ingestType;

  //see which kind of data we are dealing with first
  int numBytes = client.readBytes(&ingestType, 1);
  if (numBytes != 1)
  {
    DebugPrint("No bytes?!?\n");
    return;
  }

  switch (ingestType)
  {
    case DATA_TYPE_WEATHER:
      IngestWeatherData(client);
      break;
  }
}

void ServeWeatherData(WiFiClient& client)
{
  WeatherHeader header;
  int numBytes = client.readBytes((char*)&header, sizeof(header));
  if (numBytes != sizeof(header))
  {
    DebugPrint("Not enough header bytes?!?\n");
    return;
  }

  DebugPrintf("Got weather request with bits %hhx\n", header.m_DataIncluded);

  unsigned char out[256] = {0};
  WeatherHeader* outHeader = (WeatherHeader*)out;
  outHeader->m_DataIncluded = 0;
  unsigned char* ptr = (unsigned char*)(outHeader + 1);
  if ((header.m_DataIncluded & WEATHER_TEMP_BIT) != 0 && (m_WeatherHeader.m_DataIncluded & WEATHER_TEMP_BIT) != 0)
  {
    TemperatureData* data = (TemperatureData*)ptr;
    *data = m_TemperatureData;
    ptr += sizeof(TemperatureData);
    outHeader->m_DataIncluded |= WEATHER_TEMP_BIT;
  }
  if ((header.m_DataIncluded & WEATHER_CO2_BIT) != 0 && (m_WeatherHeader.m_DataIncluded & WEATHER_CO2_BIT) != 0)
  {
    CO2Data* data = (CO2Data*)ptr;
    *data = m_CO2Data;
    ptr += sizeof(CO2Data);
    outHeader->m_DataIncluded |= WEATHER_CO2_BIT;
  }
  if ((header.m_DataIncluded & WEATHER_PM_BIT) != 0 && (m_WeatherHeader.m_DataIncluded & WEATHER_PM_BIT) != 0)
  {
    PMData* data = (PMData*)ptr;
    *data = m_PMData;
    ptr += sizeof(PMData);
    outHeader->m_DataIncluded |= WEATHER_PM_BIT;
  }
  if ((header.m_DataIncluded & WEATHER_BATT_BIT) != 0 && (m_WeatherHeader.m_DataIncluded & WEATHER_BATT_BIT) != 0)
  {
    BatteryData* data = (BatteryData*)ptr;
    *data = m_BatteryData;
    ptr += sizeof(BatteryData);
    outHeader->m_DataIncluded |= WEATHER_BATT_BIT;
  }

  size_t outLength = (size_t)(ptr - out);
  client.write(out, outLength);
}

void DoServer(WiFiClient& client)
{
  unsigned char dataType;

  //see which kind of data is being requested
  int numBytes = client.readBytes(&dataType, 1);
  if (numBytes != 1)
  {
    DebugPrint("No bytes?!?\n");
    return;
  }

  switch (dataType)
  {
    case DATA_TYPE_WEATHER:
      ServeWeatherData(client);
      break;
  }
}

void loop()
{
  if (m_IngestSocket.hasClient())
  {
    WiFiClient client = m_IngestSocket.accept();
    client.setTimeout(2);
    DoIngest(client);
    client.stop();
  }
  if (m_ServerSocket.hasClient())
  {
    WiFiClient client = m_ServerSocket.accept();
    client.setTimeout(2);
    DoServer(client);
    client.stop();
  }
  delay(100);
}
