#pragma once

#include <MQTT.h>


class MQTTDevice
{
public:
    MQTTDevice(MQTTClient& client, String name, String id);

    MQTTClient& GetClient() { return m_Client; }
    String GetName() { return m_Name; }
    String GetID() { return m_ID; }

private:
    MQTTClient& m_Client;
    String m_Name;
    String m_ID;
};
