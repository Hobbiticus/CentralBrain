#pragma once

#include "MyMQTT.h"


class MQTTDevice
{
public:
    MQTTDevice(MyMQTT& client, String name, String id);

    MyMQTT& GetClient() { return m_Client; }
    String GetName() { return m_Name; }
    String GetID() { return m_ID; }

private:
    MyMQTT& m_Client;
    String m_Name;
    String m_ID;
};
