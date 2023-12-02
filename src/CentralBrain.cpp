#include <Arduino.h>
#include <WiFi.h>
#include <ezTime.h>
#include "creds.h"
#include "debug.h"
#include "MyTime.h"
#include "../include/IngestProtocol.h"
#include "../include/WeatherProtocol.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <ArduinoHA.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include "MQTTDevice.h"
#include "MQTTSensor.h"


IPAddress local_IP(192, 168, 1, 222);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 1);
IPAddress secondaryDNS(8, 8, 4, 4);
#define BROKER_ADDR IPAddress(192,168,1,98)

WiFiClient wifi2;
MQTTClient mqtt2(256);

WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);
HASensorNumber TemperatureSensor("temperature", HABaseDeviceType::PrecisionP1);
HASensorNumber HumiditySensor("humidity", HABaseDeviceType::PrecisionP1);
HASensorNumber PressureSensor("pressure", HABaseDeviceType::PrecisionP1);
HASensorNumber CO2Sensor("co2", HABaseDeviceType::PrecisionP0);
HASensorNumber PM10Sensor("pm10", HABaseDeviceType::PrecisionP0);
HASensorNumber PM25Sensor("pm25", HABaseDeviceType::PrecisionP0);
HASensorNumber PM01Sensor("pm01", HABaseDeviceType::PrecisionP0);
HASensorNumber BatterySensor("battery", HABaseDeviceType::PrecisionP2);

MQTTDevice DeviceWeather(mqtt2, "Weather", "weather");
MQTTSensor SensorTemperature(DeviceWeather, "Temperature", "temperature");
MQTTSensor SensorHumidity(DeviceWeather, "Humidity", "humidity");
MQTTSensor SensorPressure(DeviceWeather, "Pressure", "pressure");
MQTTSensor SensorCO2(DeviceWeather, "CO2", "co2");
MQTTSensor SensorPM10(DeviceWeather, "PM10", "pm10");
MQTTSensor SensorPM25(DeviceWeather, "PM2.5", "pm25");
MQTTSensor SensorPM01(DeviceWeather, "PM0.1", "pm01");
MQTTSensor SensorBattery(DeviceWeather, "Battery", "battery");

const unsigned short IngestPort = 7777;
const unsigned short ServerPort = 7788;
WiFiServer m_IngestSocket;
WiFiServer m_ServerSocket;

WeatherHeader m_WeatherHeader;
TemperatureData m_TemperatureData;
CO2Data m_CO2Data;
PMData m_PMData;
BatteryData m_BatteryData;
Timezone myTZ;

void messageReceived(String &topic, String &payload);
extern int ErrorLine;
int ErrorLine = 0;
extern int BuffSize;
extern int OutLen;

void setup()
{
  Serial.begin(115200);
  Serial.printf("sd begin = %hhu\n", SD.begin(5));
  TemperatureSensor.setUnitOfMeasurement("°F");
  TemperatureSensor.setDeviceClass("temperature");
  HumiditySensor.setUnitOfMeasurement("%");
  HumiditySensor.setDeviceClass("humidity");
  PressureSensor.setUnitOfMeasurement("Pa");
  PressureSensor.setDeviceClass("pressure");
  CO2Sensor.setUnitOfMeasurement("ppm");
  CO2Sensor.setDeviceClass("carbon_dioxide");
  PM10Sensor.setUnitOfMeasurement("µg/m³");
  PM10Sensor.setDeviceClass("pm10");
  PM25Sensor.setUnitOfMeasurement("µg/m³");
  PM25Sensor.setDeviceClass("pm25");
  PM01Sensor.setUnitOfMeasurement("µg/m³");
  PM01Sensor.setDeviceClass("pm1");
  BatterySensor.setUnitOfMeasurement("V");
  BatterySensor.setDeviceClass("voltage");


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
  
  byte mac[6];
  WiFi.macAddress(mac);
  device.setUniqueId(mac, sizeof(mac));
  device.setName("CentralBrain");
  device.setSoftwareVersion("1.0.0");

  bool result = mqtt.begin(BROKER_ADDR, MQTT_USER, MQTT_PASSWORD);
  Serial.printf("mqtt begin said: %d\n", result ? 1 : 0);

  //mqtt2.begin("homeassistant.local", wifi2);
  mqtt2.begin(BROKER_ADDR, wifi2);
  if (!mqtt2.connect("testtest", MQTT_USER, MQTT_PASSWORD))
  {
    Serial.println("Failed to connect to mqtt");
  }
  else
  {
    Serial.println("Connected to mqtt!! yay!!!");
    mqtt2.onMessage(messageReceived);
    SensorTemperature.Init("temperature", "°F");
    SensorHumidity.Init("humidity", "%");
    SensorPressure.Init("pressure", "Pa");
    SensorCO2.Init("carbon_dioxide", "ppm");
    SensorPM10.Init("pm10", "µg/m³");
    SensorPM25.Init("pm25", "µg/m³");
    SensorPM01.Init("pm1", "µg/m³");
    SensorBattery.Init("voltage", "V");
  }

  waitForSync();
  myTZ.setLocation("America/New_York");
  Serial.println("now = " + myTZ.dateTime());

  m_IngestSocket.begin(IngestPort);
  m_ServerSocket.begin(ServerPort);
}

void messageReceived(String &topic, String &payload)
{
  Serial.println("incoming: " + topic + " - " + payload);

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
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
    double tempF = m_TemperatureData.m_Temperature / 100.0 * 9 / 5 + 32;
    DebugPrintf("Got temperature stuff: %.1f F, %.1f%%, %.2f\n", tempF, m_TemperatureData.m_Humidity / 10.0, m_TemperatureData.m_Pressure / 100.0);
    File file = SD.open("/log.txt", FILE_APPEND);
    if (file)
    {
      file.printf("%s: Weather: %.1f F, %.1f%%, %.2f\n", myTZ.dateTime().c_str(), tempF, m_TemperatureData.m_Humidity / 10.0, m_TemperatureData.m_Pressure / 100.0);
      file.close();
    }
    m_WeatherHeader.m_DataIncluded |= WEATHER_TEMP_BIT;
    TemperatureSensor.setValue((float)tempF);
    HumiditySensor.setValue(m_TemperatureData.m_Humidity / 10.0f);
    PressureSensor.setValue(m_TemperatureData.m_Pressure / 100.0f);
    SensorTemperature.PublishValue(String(tempF, 1));
    SensorHumidity.PublishValue(String(m_TemperatureData.m_Humidity / 10.0f, 1));
    SensorPressure.PublishValue(String(m_TemperatureData.m_Pressure / 100.0f, 2));
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
    File file = SD.open("/log.txt", FILE_APPEND);
    if (file)
    {
      file.printf("%s: CO2: %hu ppm\n", myTZ.dateTime().c_str(), m_CO2Data.m_PPM);
      file.close();
    }
    m_WeatherHeader.m_DataIncluded |= WEATHER_CO2_BIT;
    CO2Sensor.setValue(m_CO2Data.m_PPM);
    SensorCO2.PublishValue(String((int)m_CO2Data.m_PPM));
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
    File file = SD.open("/log.txt", FILE_APPEND);
    if (file)
    {
      file.printf("%s: PM: pm10: %hhu, pm2.5: %hhu, pm0.1: %hhu\n", myTZ.dateTime().c_str(), m_PMData.m_10, m_PMData.m_2_5, m_PMData.m_0_1);
      file.close();
    }
    m_WeatherHeader.m_DataIncluded |= WEATHER_PM_BIT;
    PM10Sensor.setValue(m_PMData.m_10);
    PM25Sensor.setValue(m_PMData.m_2_5);
    PM01Sensor.setValue(m_PMData.m_0_1);
    SensorPM10.PublishValue(String(m_PMData.m_10));
    SensorPM25.PublishValue(String(m_PMData.m_2_5));
    SensorPM01.PublishValue(String(m_PMData.m_0_1));
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
    double volts = m_BatteryData.m_Voltage / 100.0;
    DebugPrintf("Got battery stuff: %.2f Volts\n", volts);
    File file = SD.open("/log.txt", FILE_APPEND);
    if (file)
    {
      file.printf("%s: Battery: %.2f Volts\n", myTZ.dateTime().c_str(), volts);
      file.close();
    }

    m_WeatherHeader.m_DataIncluded |= WEATHER_BATT_BIT;
    BatterySensor.setValue((float)volts);
    SensorBattery.PublishValue(String(volts, 2));
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
    DebugPrint("Adding temp data\n");
    TemperatureData* data = (TemperatureData*)ptr;
    *data = m_TemperatureData;
    ptr += sizeof(TemperatureData);
    outHeader->m_DataIncluded |= WEATHER_TEMP_BIT;
  }
  if ((header.m_DataIncluded & WEATHER_CO2_BIT) != 0 && (m_WeatherHeader.m_DataIncluded & WEATHER_CO2_BIT) != 0)
  {
    DebugPrint("Adding co2 data\n");
    CO2Data* data = (CO2Data*)ptr;
    *data = m_CO2Data;
    ptr += sizeof(CO2Data);
    outHeader->m_DataIncluded |= WEATHER_CO2_BIT;
  }
  if ((header.m_DataIncluded & WEATHER_PM_BIT) != 0 && (m_WeatherHeader.m_DataIncluded & WEATHER_PM_BIT) != 0)
  {
    DebugPrint("Adding PM data\n");
    PMData* data = (PMData*)ptr;
    *data = m_PMData;
    ptr += sizeof(PMData);
    outHeader->m_DataIncluded |= WEATHER_PM_BIT;
  }
  if ((header.m_DataIncluded & WEATHER_BATT_BIT) != 0 && (m_WeatherHeader.m_DataIncluded & WEATHER_BATT_BIT) != 0)
  {
    DebugPrint("Adding batt data\n");
    BatteryData* data = (BatteryData*)ptr;
    *data = m_BatteryData;
    ptr += sizeof(BatteryData);
    outHeader->m_DataIncluded |= WEATHER_BATT_BIT;
  }

  size_t outLength = (size_t)(ptr - out);
  DebugPrintf("Responding with %d bytes\n", outLength);
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
  DebugPrintf("Request type = %hhu\n", dataType);

  switch (dataType)
  {
    case DATA_TYPE_WEATHER:
      ServeWeatherData(client);
      break;
  }
}

void loop()
{
  mqtt.loop();
  mqtt2.loop();
  events(); //ezTime events()
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
    DebugPrint("Received client connection!\n");
    client.setTimeout(2);
    DoServer(client);
    client.stop();
  }
  delay(100);
}
