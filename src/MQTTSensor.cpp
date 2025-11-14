#include "MQTTSensor.h"
#include <ArduinoJson.h>


MQTTSensor::MQTTSensor(MQTTDevice& device, String name, String id)
: m_Device(device), m_Name(name), m_ID(id)
{
}

bool MQTTSensor::Init(String devClass, String units, int expireTimeSecs)
{
    //NOTE: the sensor ID needs to be unique across all sensors, so we are going to silently prepend the device ID to the sensor ID
    StaticJsonDocument<512> doc;
    doc["uniq_id"] = m_Device.GetID() + "_" + m_ID;
    doc["dev_cla"] = devClass;
    doc["unit_of_meas"] = units;
    auto dev = doc["dev"];
    dev["ids"] = m_Device.GetID();
    dev["name"] = m_Device.GetName();
    //dev["sw"] = "1.0.1";
    m_ValueTopic = String("mydevs/") + m_Device.GetID() + "/" + m_Device.GetID() + "_" + m_ID + "/stat_t";
    doc["stat_t"] = m_ValueTopic;
    if (expireTimeSecs > 0)
        doc["expire_after"] = expireTimeSecs;
    String output;
    serializeJson(doc, output);
    String topic = "homeassistant/sensor/" + m_Device.GetID() + "/" + m_Device.GetID() + "_" + m_ID + "/config";
    //Serial.println("TOPIC: " + topic);
    //Serial.println("VALUE: " + output);
    return m_Device.GetClient().publish(topic, output, 1, 0);
}

void LogToSD(const char* fmt, ...);

bool MQTTSensor::PublishValue(String value)
{
    Serial.printf("NEW VALUE: %s = %s\n", m_ValueTopic.c_str(), value.c_str());
    bool ret = m_Device.GetClient().publish(m_ValueTopic, value, 1, 0);
    if (!ret)
    {
        Serial.printf("Failed to publish: %d\n", m_Device.GetClient().lastError());
        LogToSD("     !!!!!!!!!!!!!!     Failed to publish: %d\n", m_Device.GetClient().lastError());
        LogToSD("Trying to reconnect....\n");
        if (m_Device.GetClient().reconnect())
        {
            Serial.printf("Reconnected!!!\n");
            LogToSD("Reconnected!!!\n");
            ret = m_Device.GetClient().publish(m_ValueTopic, value, 1, 0);
            if (ret)
            {
                Serial.printf("Yay! published goodly!\n");
                LogToSD("Yay! published goodly!\n");
            }
            else
            {
                Serial.printf("Blarg! failed to reconnect: %d\n", m_Device.GetClient().lastError());
                LogToSD("Blarg! failed to reconnect: %d\n", m_Device.GetClient().lastError());
            }
        }

    }
    return ret;
}
